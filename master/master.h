#ifndef	MASTER_H
#define	MASTER_H

class master
{
    public:
	bool start (conf &cf) ;
	bool stop (void) ;

	bool force_slave_status (slaveid_t sid, int stat) {
	    return engine_.force_slave_status (sid, stat) ;
	}

	std::string html_debug (void) ;

    private:
	sos::sos engine_ ;

	struct httpserver
	{
	    std::shared_ptr <http::server2::server> server_ ;
	    std::shared_ptr <asio::thread> threads_ ;
	} ;
	std::list <httpserver> httplist_ ;
} ;

#endif
