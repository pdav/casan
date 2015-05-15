/**
 * @file master.h
 * @brief master class interface
 */

#ifndef	MASTER_H
#define	MASTER_H

#include "conf.h"
#include "cache.h"
#include "casan.h"

namespace http {
namespace server2 {
struct reply;
struct request;
}
}

class resource ;

/**
 * @brief The master class is the main class of the CASAN master.
 *
 * The master class contains two main methods: master::start
 * and master::stop. All work is done in the various threads
 * started by master::start. The master::handle_http method
 * is called by an HTTP server thread when a request is
 * received.
 */

class master
{
    public:
	bool start (conf &cf) ;
	bool stop (void) ;

	void handle_http (const std::string request_path, const http::server2::request& req, http::server2::reply& rep);

    private:
	casan::casan engine_ ;
	conf *conf_ ;
	casan::cache cache_ ;

	struct httpserver
	{
	    std::shared_ptr <http::server2::server> server_ ;
	    std::shared_ptr <asio::thread> threads_ ;
	} ;
	std::list <httpserver> httplist_ ;

	struct parse_result
	{
	    conf::cf_ns_type type_ ;
	    std::string base_ ;		// first part of path
	    casan::slave *slave_ ;	// for NS_CASAN
	    casan::resource *res_ ;	// for NS_CASAN
	    std::string str_ ;		// for NS_ADMIN
	} ;

	void http_admin (const parse_result &res, const http::server2::request& req, http::server2::reply& rep) ;
	void http_casan (const parse_result &res, const http::server2::request& req, http::server2::reply& rep) ;
	void http_well_known (const parse_result &res, const http::server2::request& req, http::server2::reply& rep) ;
	bool parse_path (const std::string path, parse_result &res) ;

	std::string html_debug (void) ;
} ;

#endif
