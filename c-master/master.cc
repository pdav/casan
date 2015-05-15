/**
 * @file master.cc
 * @brief master class implementation
 */

#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>

#include <unistd.h>
#include <signal.h>

#include "global.h"
#include "utils.h"
#include "conf.h"			// configuration file
#include "l2.h"
#include "l2-eth.h"
#include "l2-154.h"
#include "waiter.h"
#include "msg.h"
#include "option.h"
#include "resource.h"
#include "casan.h"			// master engine
#include "cache.h"
#include "server.hpp"			// http server

#include "master.h"

/******************************************************************************
 */

/**
 * @brief Start the whole CASAN machinery
 *
 * This method:
 * - creates a casan::l2net object for each network listed in the
 *	configuration file (which implies a new thread each time)
 * - creates each CASAN slave listed in the configuration file
 * - starts the threads for the HTTP servers
 *
 * Work is next done in the various threads (HTTP or CASAN).
 *
 * @param cf result of the configuration file read (see the conf class)
 * @result true if starting was successfull
 */

bool master::start (conf &cf)
{
    bool r = true ;			// successful start

    if (cf.done_)
    {
	// Start CASAN engine machinery
	engine_.timer_first_hello (cf.timers [conf::I_FIRST_HELLO]) ;
	engine_.timer_interval_hello (cf.timers [conf::I_INTERVAL_HELLO]) ;
	engine_.timer_slave_ttl (cf.timers [conf::I_SLAVE_TTL]) ;
	engine_.init () ;

	conf_ = &cf ;

	// Start interfaces
	for (auto &n : cf.netlist_)
	{
	    casan::l2net *l ;

	    switch (n.type)
	    {
		case conf::NET_ETH :
		    {
#if defined (USE_PF_PACKET) || defined (USE_PCAP)
			casan::l2net_eth *le ;
			le = new casan::l2net_eth ;
			if (le->init (n.net_eth.iface.c_str (), n.mtu, n.net_eth.ethertype) == -1)
			{
			    perror ("init") ;
			    delete le ;
			    return 0 ;
			}
			l = le ;
			engine_.start_net (l) ;
			D (D_CONF, "Interface " << n.net_eth.iface << " initialized") ;
#endif
		    }
		    break ;
		case conf::NET_154 :
		    {
			casan::l2net_154 *l8 ;
			const char *t ;

			l8 = new casan::l2net_154 ;

			switch (n.net_154.type)
			{
			    case conf::NET_154_XBEE :
				t = "xbee" ;
				break ;
			    default :
				t = "(none)" ;
				break ;
			}

			if (l8->init (n.net_154.iface, t, n.mtu, n.net_154.addr, n.net_154.panid, n.net_154.channel) == -1)
			{
			    perror ("init") ;
			    delete l8 ;
			    return 0 ;
			}
			l = l8 ;
			engine_.start_net (l) ;
			D (D_CONF, "Interface " << n.net_154.iface << " initialized") ;
		    }
		    break ;
		default :
		    std::cerr << "(unrecognized network)\n" ;
		    break ;
	    }
	}

	// Add registered slaves
	for (auto &s : cf.slavelist_)
	{
	    casan::slave *v ;
	    casantimer_t ttl ;

	    v = new casan::slave ;
	    v->slaveid (s.id) ;
	    if (s.ttl == 0)
		ttl = engine_.timer_slave_ttl () ;
	    else
		ttl = s.ttl ;
	    v->defmtu (s.mtu) ;		// default mtu (not related to L2 spec)
	    v->init_ttl (ttl) ;
	    engine_.add_slave (v) ;
	    D (D_CONF, "Slave " << s.id << " added") ;
	}

	// Start HTTP servers
	for (auto &h : cf.httplist_)
	{
	    struct httpserver tmp ;

	    tmp.server_ = std::shared_ptr <http::server2::server>
	    	(new http::server2::server (h.listen, h.port, h.threads)) ;
	    tmp.threads_ = std::shared_ptr <asio::thread>
	    	(new asio::thread (boost::bind (&http::server2::server::run,
						tmp.server_.get ()))) ;
	    httplist_.push_back (tmp) ;
	}
    }
    else r = false ;

    return r ;
}

/**
 * @brief Stop the whole CASAN machinery
 *
 * This method stops the various threads involved in the CASAN program.
 *
 * @return true if stopping was successfull
 */

bool master::stop (void)
{
    bool r = true ;

    for (auto &h : httplist_)
	h.server_->stop () ;

    for (auto &h : httplist_)
	h.threads_->join () ;

    // engine_.stop () ;

    return r ;
}

/******************************************************************************
 * Debug
 */

std::string master::html_debug (void)
{
    return engine_.html_debug () ;
}

/******************************************************************************
 * Parse a PATH to extract namespace type, slave and resource on this slave
 */

bool master::parse_path (const std::string path, master::parse_result &res)
{
    bool r = false ;
    std::string buf = "" ;
    slaveid_t sid ;

    res.type_ = conf::NS_NONE ;
    r = true ;				// success, unless exception
    try
    {
	std::vector <std::string> vp = casan::split_path (path) ;

	for (auto &ns : conf_->nslist_)
	{
	    std::vector <std::string>::size_type i ;

	    // Search the prefix
	    for (i = 0 ; i < ns.prefix.size () ; i++)
		if (i >= vp.size () || vp [i] != ns.prefix [i])
		    break ;

	    // Is prefix found?
	    if (i == ns.prefix.size ())
	    {
		res.base_ = casan::join_path (ns.prefix) ;

		switch (ns.type)
		{
		    case conf::NS_ADMIN :
		    {
			// remaining path
			if (i == vp.size ())
			    res.str_ = "/" ;
			else {
			    res.str_ = "" ;
			    for ( ; i < vp.size () ; i++)
				res.str_ += "/" + vp [i] ;
			}

			D (D_HTTP, "HTTP request for admin namespace: " << res.base_ << ", remainder=" << res.str_) ;
			break ;
		    }
		    case conf::NS_CASAN :
		    {
			/*
			 * Find designated slave
			 */

			if (i >= vp.size ())
			    throw int (42) ;

			res.base_ += "/" + vp [i] ;
			try
			{
			    sid = std::stoi (vp [i]) ;
			}
			catch (...)
			{
			    sid = -1 ;
			}
			i++ ;

			res.slave_ = engine_.find_slave (sid) ;
			if (res.slave_ == nullptr ||
				res.slave_->status () != casan::slave::SL_RUNNING)
			    throw int (42) ;

			/*
			 * Find designated resource
			 */

			std::vector <std::string> vp2 ;
			for ( ; i < vp.size () ; i++)
			    vp2.push_back (vp [i]) ;

			res.res_ = res.slave_->find_resource (vp2) ;
			if (res.res_ == nullptr)
			    throw int (42) ;

			D (D_HTTP, "HTTP request for casan namespace: " << res.base_ << ", slave id=" << res.slave_->slaveid ()) ;

			break ;
		    }
		    case conf::NS_WELL_KNOWN :
		    {
			/*
			 * No specific handling
			 */

			break ;
		    }
		    case conf::NS_NONE :
		    default :
		    {
			throw int (42) ;
			break ;
		    }
		}

		// type
		res.type_ = ns.type ;
	    }
	}
    } catch (int e) {
	r = false ;
    }

    return r ;
}

/******************************************************************************
 */

/**
 * @brief Handle a HTTP request
 *
 * This method is called by an HTTP server thread to handle a request,
 * which may be forwarded to the CASAN side or answered directly if it
 * is a control request.
 *
 * @param request_path requested URL
 * @param req the request HTTP message
 * @param rep our reply
 */

void master::handle_http (const std::string request_path, const http::server2::request & req, http::server2::reply & rep)
{
    parse_result res ;

    if (! parse_path (request_path, res))
    {
	rep = http::server2::reply::stock_reply (http::server2::reply::not_found) ;
	return ;
    }

    switch (res.type_)
    {
	case conf::NS_ADMIN :
	    http_admin (res, req, rep) ;
	    break ;
	case conf::NS_CASAN :
	    http_casan (res, req, rep) ;
	    break ;
	case conf::NS_WELL_KNOWN :
	    http_well_known (res, req, rep) ;
	    break ;
	case conf::NS_NONE :
	default :
	    rep = http::server2::reply::stock_reply (http::server2::reply::not_found) ;
	    break ;
    }
}

/******************************************************************************
 * Handle a HTTP request for admin namespace
 */

void master::http_admin (const parse_result & res, const http::server2::request & req, http::server2::reply & rep)
{
    if (res.str_ == "/")
    {
	rep.status = http::server2::reply::ok ;
	rep.content = "<html><body><ul>"
	    "<li><a href=\"" + res.base_ + "/conf\">configuration</a>"
	    "<li><a href=\"" + res.base_ + "/run\">running status</a>"
	    "<li><a href=\"" + res.base_ + "/slave\">slave status</a>"
	    "</ul></body></html>" ;

	rep.headers.resize (2) ;
	rep.headers[0].name = "Content-Length" ;
	rep.headers[0].value =
	    boost::lexical_cast < std::string > (rep.content.size ()) ;
	rep.headers[1].name = "Content-Type" ;
	rep.headers[1].value = "text/html" ;
    }
    else if (res.str_ == "/conf")
    {
	rep.status = http::server2::reply::ok ;
	rep.content = "<html><body><pre>"
	    + conf_->html_debug () + "</pre></body></html>" ;

	rep.headers.resize (2) ;
	rep.headers[0].name = "Content-Length" ;
	rep.headers[0].value = boost::lexical_cast < std::string > (rep.content.size ()) ;
	rep.headers[1].name = "Content-Type" ;
	rep.headers[1].value = "text/html" ;
    }
    else if (res.str_ == "/run")
    {
	rep.status = http::server2::reply::ok ;
	rep.content = "<html><body><pre>"
	    + html_debug () + "</pre></body></html>" ;

	rep.headers.resize (2) ;
	rep.headers[0].name = "Content-Length" ;
	rep.headers[0].value = boost::lexical_cast < std::string > (rep.content.size ()) ;
	rep.headers[1].name = "Content-Type" ;
	rep.headers[1].value = "text/html" ;
    }
    else if (res.str_ == "/slave")
    {
	// check which slave is concerned
	if (req.method == "POST")
	{
	    enum casan::slave::status_code newstatus = casan::slave::SL_INACTIVE ;
	    slaveid_t sid = -1 ;

	    for (auto &a:req.postargs)
	    {
		if (a.name == "slaveid")
		{
		    try
		    {
			sid = std::stoi (a.value) ;
		    }
		    catch (std::exception & e)
		    {
			sid = -1 ;
		    }
		}
		else if (a.name == "status")
		{
		    if (a.value == "inactive")
			newstatus = casan::slave::SL_INACTIVE ;
		    else if (a.value == "runnning")
			newstatus = casan::slave::SL_RUNNING ;
		}
	    }
	    if (sid != -1)
	    {
		bool r = true ;

		// r = master.force_slave_status (sid, newstatus)  ;
		if (r)
		{
		    rep.status = http::server2::reply::ok ;
		    rep.content = "<html><body><pre>set to " ;
		    rep.content += (newstatus == casan::slave::SL_INACTIVE ?
					"inactive" : "running") ;
		    rep.content += "(note: not implemented)"
		    		"</pre></body></html>" ;
		}
		else
		{
		    rep.status = http::server2::reply::not_modified ;
		    rep.content = "<html><body><pre>"
			"not modified, really" "</pre></body></html>" ;
		}
	    }
	    else
	    {
		rep.status = http::server2::reply::bad_request ;
		rep.content = "<html><body><pre>"
		    "so bad request..." "</pre></body></html>" ;
	    }
	}
	else
	{
	    rep.status = http::server2::reply::ok ;
	    rep.content = "<html><body>"
		"<form method=\"post\" action=\"" + res.base_ + "/slave\">"
		"slave id <input type=text name=slaveid><p>"
		"status <select size=1 name=status>"
		"<option value=\"inactive\">INACTIVE"
		"<option value=\"running\">RUNNING"
		"</select>"
		"<input type=submit value=set>" "</pre></body></html>" ;
	}

	rep.headers.resize (2) ;
	rep.headers[0].name = "Content-Length" ;
	rep.headers[0].value =
	    boost::lexical_cast < std::string > (rep.content.size ()) ;
	rep.headers[1].name = "Content-Type" ;
	rep.headers[1].value = "text/html" ;
    }
    else
    {
	rep = http::server2::reply::stock_reply (http::server2::reply::not_found) ;
    }
}

/******************************************************************************
 * Handle a HTTP request for the "well-known" namespace
 *
 * __attribute__((unused)): gcc-specific variable attribute (which
 * means: "do not bark on me, I know this variable is not used")
 */

void master::http_well_known (const parse_result & res __attribute__ ((unused)), const http::server2::request & req __attribute__ ((unused)), http::server2::reply & rep)
{
    rep.status = http::server2::reply::ok ;
    rep.content = engine_.resource_list () ;

    rep.headers.resize (2) ;
    rep.headers[0].name = "Content-Length" ;
    rep.headers[0].value =
	boost::lexical_cast < std::string > (rep.content.size ()) ;
    rep.headers[1].name = "Content-Type" ;
    rep.headers[1].value = "text/html" ;
}

/******************************************************************************
 * Handle a HTTP request for casan namespace
 */

static struct
{
    const char *text ;
    const casan::msg::msgcode code ;
} tabmethod[] = {
    { "GET",    casan::msg::MC_GET },
    { "HEAD",   casan::msg::MC_GET },
    { "POST",   casan::msg::MC_POST },
    { "PUT",    casan::msg::MC_PUT },
    { "DELETE", casan::msg::MC_DELETE },
} ;

void master::http_casan (const parse_result &res, const http::server2::request & req, http::server2::reply & rep)
{
    std::shared_ptr <casan::msg> m (new casan::msg) ;
    std::shared_ptr <casan::msg> mc ;	// message found in cache,if any
    std::shared_ptr <casan::msg> r ;	// reply (received or cached)
    casan::msg::msgcode_t code ;
    casan::waiter w ;
    timepoint_t timeout ;
    casantimer_t max ;

    code = casan::msg::MC_GET ;
    for (int i = 0 ; i < NTAB (tabmethod); i++)
	if (tabmethod[i].text == req.method)
	    code = tabmethod[i].code ;

    m->peer (res.slave_) ;
    m->type (casan::msg::MT_CON) ;
    m->code (code) ;
    res.res_->add_to_message (*m) ;	// add resource path as msg options

    //return bad_request if request is too large for slave mtu
    //TODO: find request length!
    if (m->paylen () > res.slave_->curmtu ()) {
	rep = http::server2::reply::stock_reply (http::server2::reply::bad_request) ;
	return ;
    }

    /*
     * Is the request already present in cache?
     */

    mc = cache_.get (m) ;
    if (mc != nullptr)
    {
	/*
	 * Request is found in cache. Don't forward it again.
	 */

	m = mc ;
	D (D_CACHE, "Found request " << *m << " in cache") ;
	D (D_CACHE, "reply = " << *(m->reqrep ())) ;
    }
    else
    {
	/*
	 * Request not found in cache. We must send it
	 * and wait for a reply
	 */

	m->wt (&w) ;

	max = EXCHANGE_LIFETIME (res.slave_->l2 ()->maxlatency()) ;
	timeout = DATE_TIMEOUT_MS (max) ;
	D (D_HTTP, "HTTP request, timeout = " << max << " ms") ;

	auto a = std::bind (&casan::casan::add_request, &this->engine_, m) ;
	w.do_and_wait (a, timeout) ;

	m->wt (nullptr) ;
    }

    r = m->reqrep () ;
    if (r == nullptr)
    {
	/*
	 * No reply to our request
	 */

	rep = http::server2::reply::stock_reply (http::server2::reply::service_unavailable) ;
    }
    else
    {
	int contentformat = 0 ;		// text/plain by default
	casan::option *o ;
	int paylen ;
	char *payld ;
	payld = (char *) r->payload (&paylen) ;

	// add the request (and reply) in the cache
	if (mc == nullptr)		// request was not already in cache
	    cache_.add (m) ;

	// get content format
	r->option_reset_iterator () ;
	while ((o = r->option_next ()) != nullptr)
	{
	    if (o->optcode () == casan::option::MO_Content_Format)
	    {
		contentformat = o->optval () ;
		break ;
	    }
	}

	// get announced mtu
	r->option_reset_iterator () ;
	while ((o = r->option_next ()) != nullptr)
	{
	    if (o->optcode () == casan::option::MO_Size1)
	    {
		res.slave_->curmtu (o->optval ()) ;
		break ;
	    }
	}

	rep.status = http::server2::reply::ok ;

	rep.headers.resize (2) ;
	rep.headers[0].name = "Content-Length" ;
	rep.headers[1].name = "Content-Type" ;

	// See draft-ietf-core-coap-16.txt:  "12.3. Content-Format Registry"
	switch (contentformat)
	{
	    case 0 :			// text/plain charset=utf-8
		// rep.content = "<html><body><pre>" ;
		for (int i = 0 ; i < paylen ; i++)
		    rep.content.push_back (payld [i]) ;
		// rep.content += "</pre></body></html>" ;
		rep.headers[1].value = "text/plain" ;
		break ;

	    case 50 :			// application/json
		// rep.content = "<html><body><pre>" ;
		for (int i = 0 ; i < paylen ; i++)
		    rep.content.push_back (payld [i]) ;
		// rep.content += "</pre></body></html>" ;
		rep.headers[1].value = "application/json" ;
		break ;
	}

	rep.headers[0].value =
	    boost::lexical_cast < std::string > (rep.content.size ()) ;
    }
}
