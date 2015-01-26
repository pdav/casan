/**
 * @file conf.h
 * @brief Configuration file handling
 */

#ifndef CONF_H
#define CONF_H

#include "global.h"

// #define	CONF_PATH	"/etc/casand.conf"
#define	CONF_PATH	"./casand.conf"

/**
 * @brief Configuration file handling
 *
 * The conf class is used to read the configuration file and
 * represent the configuration through an intermediate form,
 * which is composed of instance variables that the master
 * class can use.
 */

class conf
{
    public:
	bool parse (void) ;
	bool parse (const std::string &file) ;

	friend std::ostream& operator<< (std::ostream &os, const conf &cf) ;

	enum cf_ns_type { NS_NONE, NS_ADMIN, NS_CASAN, NS_WELL_KNOWN } ;

    protected:
	friend class master ;

	std::string html_debug (void) ;

	bool done_ = false ;		///< configuration file already read

	/// timer values
	enum cf_timer_index {
	    I_FIRST_HELLO = 0,		///< default slave ttl
	    I_INTERVAL_HELLO = 1,	///< interval between hello
	    I_SLAVE_TTL = 2,		///< default slave ttl
	    I_LAST = 3			///< last value
	} ;
	casantimer_t timers [I_LAST] ;

	/// HTTP server configuration
	struct cf_http
	{
	    std::string listen ;	///< listen address (v4 or v6)
	    std::string port ;		///< tcp port number or name
	    int threads = 0 ;		///< number of threads for this server
	} ;
	std::list <cf_http> httplist_ ;

	/// namespace configuration
	struct cf_namespace
	{
	    std::vector <std::string> prefix ;	///< namespace prefix
	    cf_ns_type type = NS_NONE ;
	} ;
	std::list <cf_namespace> nslist_ ;

	/// interface type
	enum net_type { NET_NONE, NET_ETH, NET_154 } ;
	/// Ethernet interface configuration
	struct cf_net_eth
	{
	    std::string iface ;		///< interface name (see `netstat -i`)
	    int ethertype = 0 ;		///< frame type
	} ;
	/// IEEE 802.15.4 type (i.e. type of interface)
	enum net_154_type { NET_154_NONE, NET_154_XBEE } ;
	/// IEEE 802.15.4 interface configuration
	struct cf_net_154
	{
	    std::string iface ;		///< entry in `/dev`
	    net_154_type type ;		///< type of driver
	    std::string addr ;		///< short or long addr: ab:cd[:ef:ab:cd:ef:ab:cd]
	    std::string panid ;		///< in hex [ab:cd]
	    int channel ;
	} ;
	/// network configuration
	struct cf_network
	{
	    net_type type = NET_NONE ;
	    int mtu = 0 ;
	    // not using an union since C++ cannot know how to initialize it
	    cf_net_eth net_eth ;
	    cf_net_154 net_154 ;
	} ;
	std::list <cf_network> netlist_ ;

	/// slave configuration
	struct cf_slave
	{
	    int id = 0 ;
	    int ttl = 0 ;
	    int mtu = 0 ;
	} ;
	std::list <cf_slave> slavelist_ ;

    private:
	std::string file_ ;		// parsed file
	int lineno_ = 0 ;		// parsed line

	bool parse_file (void) ;
	bool parse_line (std::string &line) ;
	void parse_error (const std::string msg, int help) ;
	void parse_error_num_token (const int n, int help) ;
	void parse_error_dup_token (const std::string tok, int help) ;
	void parse_error_unk_token (const std::string tok, int help) ;
	void parse_error_mis_token (int help) ;

	const casantimer_t DEFAULT_FIRST_HELLO	= 3 ;		// 3 s
	const casantimer_t DEFAULT_INTERVAL_HELLO = 10 ;	// 10 s
	const casantimer_t DEFAULT_SLAVE_TTL	= 3600 ;	// 1 h

	const char *DEFAULT_HTTP_PORT		= "http" ;
	const char *DEFAULT_HTTP_LISTEN		= "*" ;
	const int DEFAULT_HTTP_THREADS		= 5 ;
	const int DEFAULT_154_CHANNEL	 	= 12 ;
} ;

#endif
