#include <iostream>
#include <string>
#include <asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <thread>

#include <unistd.h>
#include <signal.h>

#include "conf.h"
#include "sos.h"

#include "server.hpp"

conf cf ;

void block_all_signals (sigset_t *old)
{
    sigset_t m ;

    sigfillset (&m) ;
    pthread_sigmask (SIG_BLOCK, &m, old) ;
}

void undo_block_all_signals (sigset_t *old)
{
    pthread_sigmask (SIG_SETMASK, old, NULL) ;
}

void wait_for_signal (void)
{
    sigset_t m ;
    int signo ;

    sigemptyset (&m) ;
    sigaddset (&m, SIGINT) ;
    sigaddset (&m, SIGQUIT) ;
    sigaddset (&m, SIGTERM) ;
    pthread_sigmask (SIG_BLOCK, &m, 0) ;
    signo = 0 ;
    sigwait (&m, &signo) ;
}

int main (int argc, char *argv [])
{
    // http::server2::server *hs ;		// XXXXXXXXXXXXX
    char *conffile ;
    sigset_t mask ;

    try
    {
	if (argc != 2)
	{
	    std::cerr << "usage: " << argv [0] << " conf-file\n" ;
	    exit (1) ;
	}
	conffile = argv [1] ;

	if (! cf.init (conffile))
	{
	    std::cout << "Read error for " << conffile << "\n" ;
	    std::cout << "Abort\n" ;
	    exit (1) ;
	}

	std::cout << cf ;

	block_all_signals (&mask) ;

	// start SOS engine
	cf.start () ;

	// start HTTP server
	std::size_t nthreads = 5 ;	// XXX should be in sos.conf
	http::server2::server hs ("127.0.0.1", "8000", nthreads) ;
	asio::thread ht (boost::bind (&http::server2::server::run, &hs)) ;

	undo_block_all_signals (&mask) ;
	wait_for_signal () ;

	hs.stop () ;
	ht.join () ;
    }
    catch (std::exception &e)
    {
	std::cerr << "Exception: " << e.what () << "\n" ;
    }

    exit (0) ;
}
