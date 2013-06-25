#ifndef	MASTER_H
#define	MASTER_H

namespace http {
namespace server2 {
struct reply;
struct request;
}
}

class resource ;

class master
{
    public:
	bool start (conf &cf) ;
	bool stop (void) ;

	std::string html_debug (void) ;

	struct parse_result
	{
	    conf::cf_ns_type type_ ;
	    std::string base_ ;		// first part of path
	    sos::slave *slave_ ;	// for NS_SOS
	    sos::resource *res_ ;	// for NS_SOS
	    std::string str_ ;		// for NS_ADMIN
	} ;

	bool parse_path (const std::string path, parse_result &res) ;
	void handle_http (const std::string request_path, const http::server2::request& req, http::server2::reply& rep);

    private:
	sos::sos engine_ ;
	conf *conf_ ;

	struct httpserver
	{
	    std::shared_ptr <http::server2::server> server_ ;
	    std::shared_ptr <asio::thread> threads_ ;
	} ;
	std::list <httpserver> httplist_ ;

	void http_admin (const parse_result &res, const http::server2::request& req, http::server2::reply& rep) ;
	void http_sos (const parse_result &res, const http::server2::request& req, http::server2::reply& rep) ;
	void http_well_known (const parse_result &res, const http::server2::request& req, http::server2::reply& rep) ;
} ;

#endif
