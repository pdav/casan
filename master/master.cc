#include <iostream>
#include <string>
#include <asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <thread>
#include <memory>

#include <unistd.h>
#include <signal.h>

#include "global.h"
#include "conf.h"			// configuration file
#include "l2.h"
#include "l2eth.h"
#include "sos.h"			// master engine
#include "server.hpp"			// http server

#include "master.h"

/******************************************************************************
 * Start the whole SOS machinery
 * - master SOS code
 * - http server
 */

bool master::start (conf &cf)
{
    bool r = true ;			// successful start

    if (cf.done_)
    {
	// Start SOS engine machinery
	engine_.ttl (cf.def_ttl_) ;
	engine_.init () ;

	// Start interfaces
	for (auto &n : cf.netlist_)
	{
	    sos::l2net *l ;

	    switch (n.type)
	    {
		case conf::NET_ETH :
		    l = new sos::l2net_eth ;
		    if (l->init (n.net_eth.iface.c_str ()) == -1)
		    {
			perror ("init") ;
			delete l ;
			return 0 ;
		    }
		    engine_.start_net (l) ;
		    std::cout << "Interface " << n.net_eth.iface << " initialized\n" ;
		    break ;
		case conf::NET_802154 :
		    std::cerr << "802.15.4 not supported\n" ;
		    break ;
		default :
		    std::cerr << "(unrecognized network)\n" ;
		    break ;
	    }
	}

	// Add registered slaves
	for (auto &s : cf.slavelist_)
	{
	    sos::slave *v ;

	    v = new sos::slave ;
	    v->slaveid (s.id) ;
	    engine_.add_slave (v) ;
	    std::cout << "Slave " << s.id << " added\n" ;
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

#if 0

masterhttp::~masterhttp ()
{
    if (server_)
	delete server_ ;
    if (threads_)
	delete threads_ ;
}

#endif
