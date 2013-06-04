#include <iostream>
#include <fstream>
#include <istream>
#include <sstream>
#include <algorithm>
#include <iterator>

#include "conf.h"
#include "l2.h"
#include "l2eth.h"
#include "sos.h"

bool conf::init (void)
{
    file_ = CONF_PATH ;
    return conf::parse_file () ;
}

bool conf::init (const std::string &file)
{
    file_ = file ;
    return conf::parse_file () ;
}

sos::sos *conf::start (void)
{
    sos::sos *e ;

    if (done_)
    {
	e = new sos::sos ;

	// Start SOS engine machinery
	e->ttl (def_ttl_) ;
	e->init () ;

	// Start interfaces
	for (auto &n : netlist_)
	{
	    sos::l2net *l ;

	    switch (n.type)
	    {
		case NET_ETH :
		    l = new sos::l2net_eth ;
		    if (l->init (n.net_eth.iface.c_str ()) == -1)
		    {
			perror ("init") ;
			delete l ;
			delete e ;
			return 0 ;
		    }
		    e->start_net (l) ;
		    std::cout << "Interface " << n.net_eth.iface << " initialized\n" ;
		    break ;
		case NET_802154 :
		    std::cerr << "802.15.4 not supported\n" ;
		    break ;
		default :
		    std::cerr << "(unrecognized network)\n" ;
		    break ;
	    }
	}

	// Add registered slaves
	for (auto &s : slavelist_)
	{
	    sos::slave *v ;

	    v = new sos::slave ;
	    v->slaveid (s.id) ;
	    e->add_slave (v) ;
	    std::cout << "Slave " << s.id << " added\n" ;
	}
    }
    else e = 0 ;

    return e ;
}


/******************************************************************************
 * Configuration file dumper
 */

std::ostream& operator<< (std::ostream &os, const conf &cf)
{
    if (cf.done_)
    {
	for (auto &h : cf.httplist_)
	    os << "http-server"
		<< " listen " << (h.listen == "" ? "*" : h.listen)
	    	<< " port " << (h.port == "" ? "80" : h.port) << "\n" ;
	for (auto &n : cf.nslist_)
	    os << "namespace "
		<< (n.type == conf::NS_ADMIN ? "admin" : "sos")
	    	<< n.prefix << "\n" ;
	for (auto &n : cf.netlist_)
	{
	    os << "network " ;
	    switch (n.type)
	    {
		case conf::NET_ETH :
		    os << "ethernet " << n.net_eth.iface << "\n" ;
		    break ;
		case conf::NET_802154 :
		    os << "802.15.4 " << n.net_eth.iface << "\n" ;
		    break ;
		default :
		    os << "(unrecognized network)\n" ;
		    break ;
	    }
	}
	if (cf.def_ttl_ != 0)
	    os << "slave-ttl " << cf.def_ttl_ << "\n" ;
	for (auto &s : cf.slavelist_)
	    os << "slave id " << s.id << " ttl " << s.ttl << "\n" ;
    }
    return os ;
}

std::string conf::html_debug (void)
{
    std::ostringstream oss ;

    oss << "<html><body><pre>\n" << *this << "</pre></body></html>\n" ;

    return oss.str () ;
}

/******************************************************************************
 * Configuration file parser
 */

#define	HELP_ALL	0
#define	HELP_HTTP	1
#define	HELP_NAMESPACE	2
#define	HELP_SLAVETTL	3
#define	HELP_NETWORK	4
#define	HELP_SLAVE	5
#define	HELP_NETETH	(HELP_SLAVE+1)
#define	HELP_NET802154	(HELP_NETETH+1)

static const char *syntax_help [] =
{
    "http-server, namespace, slave-ttl, network, or slave",

    "http-server [listen <addr>] [port <num>]",
    "namespace <admin|sos> <path>",
    "slave-ttl <timeout in s>",
    "network <ethernet|802.15.4> ...",
    "slave id <id> [ttl <timeout in s>] [mtu <bytes>]",

    "network ethernet <iface> [mtu <bytes>]",
    "network 802.15.4 <iface> [mtu <bytes>] ???",
} ;

bool conf::parse_file (void)
{
    std::ifstream f (file_) ;
    std::string line ;
    bool r ;

    r = true ;				// everything went well
    if (f.is_open ())
    {
	lineno_ = 0 ;

	while (getline (f, line))
	{
	    lineno_++ ;
	    r = conf::parse_line (line) ;
	    if (! r)
		break ;
	}
	if (f.bad ())
	    r = false ;
	f.close () ;
    }
    else r = false ;

    if (r)
	done_ = true ;

    return r ;
}

bool conf::parse_line (std::string &line)
{
    std::vector <std::string> tokens ;

    /*
     * First pass: remove comments
     */

    std::string::size_type sharp ;

    sharp = line.find ('#') ;		// search starting from position 0
    if (sharp != std::string::npos)
	line.erase (sharp) ;

    /*
     * Second pass: split line into tokens
     */

    std::stringstream ss (line) ;
    std::string buf ;

    while (ss >> buf)			// read word by word
    {
	tokens.push_back (buf) ;
    }

    /*
     * Third pass: quick & dirty line parser
     */

    int asize ;				// vector size
    int i ;				// current index in vector
    int r = true ;

    asize = tokens.size () ;
    i = 0 ;

    if (i < asize)
    {
	if (tokens [i] == "http-server")
	{
	    cf_http c ;

	    c.listen = "" ;
	    c.port = "" ;

	    i++ ;
	    for ( ; i + 1 < asize ; i += 2)
	    {
		if (tokens [i] == "listen")
		{
		    if (c.listen != "")
		    {
			parse_error_dup_token (tokens [i], HELP_HTTP) ;
			r = false ;
			break ;
		    }
		    else c.listen = tokens [i+1] ;
		}
		else if (tokens [i] == "port")
		{
		    if (c.port != "")
		    {
			parse_error_dup_token (tokens [i], HELP_HTTP) ;
			r = false ;
			break ;
		    }
		    else c.port = tokens [i+1] ;
		}
		else
		{
		    parse_error_unk_token (tokens [i], HELP_HTTP) ;
		    r = false ;
		    break ;
		}
	    }
	    if (r)
	    {
		if (i != asize)		// odd number of parameters
		{
		    parse_error_num_token (asize, HELP_HTTP) ;
		    r = false ;
		}
		else httplist_.push_back (c) ;
	    }
	}
	else if (tokens [i] == "namespace")
	{
	    cf_namespace c ;

	    c.type = NS_NONE ;
	    c.prefix = "" ;

	    // "namespace <admin|sos> <path>",

	    i++ ;
	    if (i + 2 != asize)
	    {
		parse_error_num_token (asize, HELP_NAMESPACE) ;
		r = false ;
	    }
	    else
	    {
		c.prefix = tokens [i+1] ;
		if (tokens [i] == "admin")
		    c.type = NS_ADMIN ;
		else if (tokens [i] == "sos")
		    c.type = NS_SOS ;
		else
		{
		    parse_error_unk_token (tokens [i], HELP_NAMESPACE) ;
		    r = false ;
		}
		if (r)
		    nslist_.push_back (c) ;
	    }
	}
	else if (tokens [i] == "slave-ttl")
	{
	    i++ ;

	    if (i + 1 != asize)
	    {
		parse_error_num_token (asize, HELP_SLAVETTL) ;
		r = false ;
	    }
	    else if (def_ttl_ != 0)
	    {
		parse_error_dup_token (tokens [0], HELP_SLAVETTL) ;
		r = false ;
	    }
	    else
	    {
		def_ttl_ = std::stoi (tokens [i]) ;
	    }
	}
	else if (tokens [i] == "network")
	{
	    cf_network c ;

	    c.type = NET_NONE ;
	    c.mtu = 0 ;

	    i++ ;
	    if (i < asize)
	    {
		if (tokens [i] == "ethernet")
		{
		    c.type = NET_ETH ;
		    i++ ;

		    if (i < asize)
		    {
			c.net_eth.iface = tokens [i] ;
			i++ ;

			for ( ; i + 1 < asize ; i += 2)
			{
			    if (tokens [i] == "mtu")
			    {
				if (c.mtu != 0)
				{
				    parse_error_dup_token (tokens [i], HELP_NETETH) ;
				    r = false ;
				    break ;
				}
				else c.mtu = std::stoi (tokens [i+1]) ;
			    }
			    else
			    {
				parse_error_unk_token (tokens [i], HELP_NETETH) ;
				r = false ;
				break ;
			    }
			}
		    }
		    else
		    {
			parse_error_num_token (asize, HELP_NETETH) ;
			r = false ;
		    }

		    if (r)
		    {
			if (i != asize)		// odd number of parameters
			{
			    parse_error_num_token (asize, HELP_NETETH) ;
			    r = false ;
			}
		    }
		}
		else if (tokens [i] == "802.15.4")
		{
		    c.type = NET_802154 ;
		    i++ ;

		    if (i < asize)
		    {
			c.net_802154.iface = tokens [i] ;
			i++ ;

			for ( ; i + 1 < asize ; i += 2)
			{
			    if (tokens [i] == "mtu")
			    {
				if (c.mtu != 0)
				{
				    parse_error_dup_token (tokens [i], HELP_NET802154) ;
				    r = false ;
				    break ;
				}
				else c.mtu = std::stoi (tokens [i+1]) ;
			    }
			    else
			    {
				parse_error_unk_token (tokens [i], HELP_NET802154) ;
				r = false ;
				break ;
			    }
			}
		    }
		    else
		    {
			parse_error_num_token (asize, HELP_NET802154) ;
			r = false ;
		    }

		    if (r)
		    {
			if (i != asize)		// odd number of parameters
			{
			    parse_error_num_token (asize, HELP_NET802154) ;
			    r = false ;
			}
		    }

		    // XXX
		    r = false ;
		}
		else
		{
		    parse_error_unk_token (tokens [i], HELP_NETWORK) ;
		    r = false ;
		}

		if (r)
		    netlist_.push_back (c) ;
	    }
	    else parse_error_mis_token (HELP_NETWORK) ;
	}
	else if (tokens [i] == "slave")
	{
	    cf_slave c ;

	    c.id = -1 ;
	    c.ttl = 0 ;
	    c.mtu = 0 ;

	    i++ ;
	    for ( ; i + 1 < asize ; i += 2)
	    {
		if (tokens [i] == "id")
		{
		    if (c.id != -1)
		    {
			parse_error_dup_token (tokens [i], HELP_SLAVE) ;
			r = false ;
			break ;
		    }
		    else c.id = std::stoi (tokens [i+1]) ;
		}
		else if (tokens [i] == "ttl")
		{
		    if (c.ttl != 0)
		    {
			parse_error_dup_token (tokens [i], HELP_SLAVE) ;
			r = false ;
			break ;
		    }
		    else c.ttl = std::stoi (tokens [i+1]) ;
		}
		else if (tokens [i] == "mtu")
		{
		    if (c.mtu != 0)
		    {
			parse_error_dup_token (tokens [i], HELP_SLAVE) ;
			r = false ;
			break ;
		    }
		    else c.mtu = std::stoi (tokens [i+1]) ;
		}
		else
		{
		    parse_error_unk_token (tokens [i], HELP_SLAVE) ;
		    r = false ;
		    break ;
		}
	    }
	    if (r)
	    {
		if (i != asize)		// odd number of parameters
		{
		    parse_error_num_token (asize, HELP_SLAVE) ;
		    r = false ;
		}
		else slavelist_.push_back (c) ;
	    }
	}
	else
	{
	    parse_error_unk_token (tokens [i], HELP_ALL) ;
	    r = false ;
	}
    }

    /*
     * Fourth pass: set default values
     */

    for (auto &s : slavelist_)
	if (s.ttl == 0)
	    s.ttl = def_ttl_ ;

    if (r)
	done_ = true ;

    return r ;
}

void conf::parse_error (const std::string msg, int help)
{
    std::cerr << file_ << "(" << lineno_ << "): " << msg << "\n" ;
    if (help >= 0)
	std::cerr << "\tusage: " << syntax_help [help] << "\n" ;
}

void conf::parse_error_num_token (const int n, int help)
{
    std::string msg ;

    msg = "invalid number of tokens (" + std::to_string (n) + ")" ;
    parse_error (msg, help) ;
}

void conf::parse_error_unk_token (const std::string tok, int help)
{
    std::string msg ;

    msg = "unrecognized token '" + tok + "'" ;
    parse_error (msg, help) ;
}

void conf::parse_error_dup_token (const std::string tok, int help)
{
    std::string msg ;

    msg = "duplicate token '" + tok + "'" ;
    parse_error (msg, help) ;
}

void conf::parse_error_mis_token (int help)
{
    std::string msg ;

    msg = "missing token/value" ;
    parse_error (msg, help) ;
}
