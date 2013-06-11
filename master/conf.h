#ifndef CONF_H
#define CONF_H

#include "global.h"

// #define	CONF_PATH	"/etc/sosd.conf"
#define	CONF_PATH	"./sosd.conf"

/*
 * Configuration file handling
 */

class conf
{
    public:
	bool parse (void) ;
	bool parse (const std::string &file) ;

	friend std::ostream& operator<< (std::ostream &os, const conf &cf) ;

	std::string html_debug (void) ;

	enum cf_ns_type { NS_NONE, NS_ADMIN, NS_SOS } ;

    protected:
	friend class master ;

	bool done_ = false ;		// configuration file already read

	// HTTP server configuration
	struct cf_http
	{
	    std::string listen ;	// listen address (v4 or v6)
	    std::string port ;		// tcp port number or name
	    int threads ;		// number of threads for this server
	} ;
	std::list <cf_http> httplist_ ;

	// namespace configuration
	struct cf_namespace
	{
	    std::string prefix ;	// namespace prefix
	    cf_ns_type type ;
	} ;
	std::list <cf_namespace> nslist_ ;

	// interface configuration
	enum net_type { NET_NONE, NET_ETH, NET_802154 } ;
	struct cf_net_eth
	{
	    std::string iface ;
	} ;
	struct cf_net_802154
	{
	    std::string iface ;
	} ;
	struct cf_network
	{
	    net_type type ;
	    int mtu ;
	    // not using an union since C++ cannot know how to initialize it
	    cf_net_eth    net_eth ;
	    cf_net_802154 net_802154 ;
	} ;
	std::list <cf_network> netlist_ ;

	// slave configuration
	int def_ttl_ = 0 ;		// default ttl
	struct cf_slave
	{
	    int id ;
	    int ttl ;
	    int mtu ;
	} ;
	std::list <cf_slave> slavelist_ ;

    private:
	std::string file_ ;		// parsed file
	int lineno_ ;			// parsed line

	bool parse_file (void) ;
	bool parse_line (std::string &line) ;
	void parse_error (const std::string msg, int help) ;
	void parse_error_num_token (const int n, int help) ;
	void parse_error_dup_token (const std::string tok, int help) ;
	void parse_error_unk_token (const std::string tok, int help) ;
	void parse_error_mis_token (int help) ;
} ;

#endif
