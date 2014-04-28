/**
 * @file conf.cc
 * @brief conf class implementation
 */

#include <iostream>
#include <fstream>
#include <istream>
#include <sstream>
#include <algorithm>
#include <iterator>

#include <string>
#include <list>
#include <vector>

#include "global.h"
#include "utils.h"

#include "conf.h"
#include "sos.h"

/**
 * @brief Parses the default configuration file
 */

bool conf::parse (void)
{
    file_ = CONF_PATH ;
    return conf::parse_file () ;
}

/**
 * @brief Parses the given configuration file
 */

bool conf::parse (const std::string &file)
{
    file_ = file ;
    return conf::parse_file () ;
}

/**
 * @brief Configuration file dumper
 *
 * This operator dumps the intermediate representation of the
 * configuration file to the output stream (as a simili configuration
 * file).
 */

std::ostream& operator<< (std::ostream &os, const conf &cf)
{
    if (cf.done_)
    {
	for (auto &h : cf.httplist_)
	    os << "http-server"
		<< " listen " << h.listen
	    	<< " port " << h.port
		<< " threads " << h.threads
		<< "\n" ;
	for (auto &n : cf.nslist_)
	{
	    os << "namespace "
		<< (n.type == conf::NS_ADMIN ? "admin" :
		    (n.type == conf::NS_SOS ? "sos" :
		    (n.type == conf::NS_WELL_KNOWN ?  "well-known" :
			"(unknown)")))
		<< " " ;
	    if (n.prefix.size () == 0)
		os << '/' ;
	    for (auto &p : n.prefix)
	    	os << '/' << p ;
	    os << "\n" ;
	}
	for (int i = 0 ; i < NTAB (cf.timers) ; i++)
	{
	    const char *p ;
	    switch (i)
	    {
		case conf::I_FIRST_HELLO :
		    p = "firsthello" ;
		    break ;
		case conf::I_INTERVAL_HELLO :
		    p = "hello" ;
		    break ;
		case conf::I_SLAVE_TTL :
		    p = "slavettl" ;
		    break ;
	    }
	    os << "timer " << p << " " << cf.timers [i] << "\n" ;
	}
	for (auto &n : cf.netlist_)
	{
	    os << "network " ;
	    switch (n.type)
	    {
		case conf::NET_ETH :
		    os << "ethernet " << n.net_eth.iface
			<< " ethertype 0x"
				<< std::hex << n.net_eth.ethertype << std::dec ;
		    break ;
		case conf::NET_802154 :
		    os << "802.15.4" << n.net_eth.iface
			<< " type " << (n.net_802154.type == conf::NET_802154_XBEE ? "xbee" : "(none)")
			<< " addr " << n.net_802154.addr
			<< " panid " << n.net_802154.panid
			<< " channel " << n.net_802154.channel
			;
		    break ;
		default :
		    os << "(unrecognized network)\n" ;
		    break ;
	    }
	    os << " mtu " << n.mtu << "\n" ;
	}
	for (auto &s : cf.slavelist_)
	    os << "slave id " << s.id
		<< " ttl " << s.ttl
		<< " mtu " << s.mtu
		<< "\n" ;
    }
    return os ;
}

/**
 * @brief Configuration file dumper
 *
 * This operator dumps the intermediate representation of the
 * configuration file to the output stream (as a simili configuration
 * file in HTML).
 */

std::string conf::html_debug (void)
{
    std::ostringstream oss ;

    oss << *this ;
    return oss.str () ;
}

/******************************************************************************
 * Configuration file parser
 */

#define	HELP_ALL	0
#define	HELP_HTTP	1
#define	HELP_NAMESPACE	2
#define	HELP_TIMER	3
#define	HELP_NETWORK	4
#define	HELP_SLAVE	5
#define	HELP_NETETH	(HELP_SLAVE+1)
#define	HELP_NET802154	(HELP_NETETH+1)

static const char *syntax_help [] =
{
    "http-server, namespace, timer, network, or slave",

    "http-server [listen <addr>] [port <num>] [threads <num>]",
    "namespace <admin|sos|well-known> <path>",
    "timer <firsthello|hello|slavettl|http> <value in s>",
    "network <ethernet|802.15.4> ...",
    "slave id <id> [ttl <timeout in s>] [mtu <bytes>]",

    "network ethernet <iface> [mtu <bytes>] [ethertype [0x]<val>]",
    "network 802.15.4 <iface> type <xbee> addr <addr> panid <id> [channel <chan>] [mtu <bytes>]",
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
	    c.threads = 0 ;

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
		else if (tokens [i] == "threads")
		{
		    if (c.threads != 0)
		    {
			parse_error_dup_token (tokens [i], HELP_HTTP) ;
			r = false ;
			break ;
		    }
		    else c.threads = std::stoi (tokens [i+1]) ;
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

	    // "namespace <admin|sos> <path>",

	    i++ ;
	    if (i + 2 != asize)
	    {
		parse_error_num_token (asize, HELP_NAMESPACE) ;
		r = false ;
	    }
	    else
	    {
		c.prefix = sos::split_path (tokens [i+1]) ;
		if (tokens [i] == "admin")
		    c.type = NS_ADMIN ;
		else if (tokens [i] == "sos")
		    c.type = NS_SOS ;
		else if (tokens [i] == "well-known")
		    c.type = NS_WELL_KNOWN ;
		else
		{
		    parse_error_unk_token (tokens [i], HELP_NAMESPACE) ;
		    r = false ;
		}
		if (r)
		    nslist_.push_back (c) ;
	    }
	}
	else if (tokens [i] == "timer")
	{
	    i++ ;

	    if (i + 2 != asize)
	    {
		parse_error_num_token (asize, HELP_TIMER) ;
		r = false ;
	    }
	    else
	    {
		int idx ;

		if (tokens [i] == "firsthello")
		    idx = I_FIRST_HELLO ;
		else if (tokens [i] == "hello")
		    idx = I_INTERVAL_HELLO ;
		else if (tokens [i] == "slavettl")
		    idx = I_SLAVE_TTL ;
		else
		    idx = -1 ;

		if (idx == -1)
		{
		    parse_error_unk_token (tokens [i], HELP_TIMER) ;
		    r = false ;
		}
		else if (timers [idx] != 0)
		{
		    i++ ;
		    timers [idx] = std::stoi (tokens [i]) ;
		}
		else
		{
		    parse_error_dup_token (tokens [0], HELP_TIMER) ;
		    r = false ;
		}
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
			    else if (tokens [i] == "ethertype")
			    {
				if (c.net_eth.ethertype != 0)
				{
				    parse_error_dup_token (tokens [i], HELP_NETETH) ;
				    r = false ;
				    break ;
				}
				else
				{
				    std::istringstream is (tokens [i+1]) ;

				    if (tokens [i+1].substr (0, 2) == "0x")
					is >> std::hex >> c.net_eth.ethertype ;
				    else
					is >> c.net_eth.ethertype ;
				}
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
		    c.net_802154.type = NET_802154_NONE ;
		    c.net_802154.addr = "" ;
		    c.net_802154.panid = "" ;
		    c.net_802154.channel = 0 ;
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
			    else if (tokens [i] == "type")
			    {
				if (c.net_802154.type != NET_802154_NONE)
				{
				    parse_error_dup_token (tokens [i], HELP_NET802154) ;
				    r = false ;
				    break ;
				}
				else
				{
				    if (tokens [i+1] == "xbee")
					c.net_802154.type = NET_802154_XBEE ;
				    else
				    {
					parse_error_unk_token (tokens [i+1], HELP_NET802154) ;
					r = false ;
					break ;
				    }
				}
			    }
			    else if (tokens [i] == "addr")
			    {
				if (c.net_802154.addr != "")
				{
				    parse_error_dup_token (tokens [i], HELP_NET802154) ;
				    r = false ;
				    break ;
				}
				else c.net_802154.addr = tokens [i+1] ;
			    }
			    else if (tokens [i] == "panid")
			    {
				if (c.net_802154.panid != "")
				{
				    parse_error_dup_token (tokens [i], HELP_NET802154) ;
				    r = false ;
				    break ;
				}
				else c.net_802154.panid = tokens [i+1] ;
			    }
			    else if (tokens [i] == "channel")
			    {
				if (c.net_802154.channel != 0)
				{
				    parse_error_dup_token (tokens [i], HELP_NET802154) ;
				    r = false ;
				    break ;
				}
				else c.net_802154.channel = std::stoi (tokens [i+1]) ;
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

	    c.id = 0 ;
	    c.ttl = 0 ;
	    c.mtu = 0 ;

	    i++ ;
	    for ( ; i + 1 < asize ; i += 2)
	    {
		if (tokens [i] == "id")
		{
		    if (c.id != 0)
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

    for (auto &h : httplist_)
    {
	if (h.port == "")
	    h.port = DEFAULT_HTTP_PORT ;
	if (h.threads == 0)
	    h.threads = DEFAULT_HTTP_THREADS ;
	if (h.listen == "")
	    h.listen = DEFAULT_HTTP_LISTEN ;
    }

    if (timers [I_FIRST_HELLO] == 0)
	timers [I_FIRST_HELLO] = DEFAULT_FIRST_HELLO ;
    if (timers [I_INTERVAL_HELLO] == 0)
	timers [I_INTERVAL_HELLO] = DEFAULT_INTERVAL_HELLO ;
    if (timers [I_SLAVE_TTL] == 0)
	timers [I_SLAVE_TTL] = DEFAULT_SLAVE_TTL ;

    for (auto &s : slavelist_)
	if (s.ttl == 0)
	    s.ttl = timers [I_SLAVE_TTL] ;

    for (auto &n : netlist_)
    {
	if (n.type == NET_ETH && n.net_eth.ethertype == 0)
	    n.net_eth.ethertype = ETHTYPE_SOS ;
	if (n.type == NET_802154 && n.net_802154.channel == 0)
	    n.net_802154.channel = DEFAULT_802154_CHANNEL ;
    }

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
