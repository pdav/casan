#include <iostream>
#include <string>
#include <list>

#include <unistd.h>
#include <signal.h>

#include "server.hpp"

#include "conf.h"
#include "sos.h"
#include "master.h"

conf cf ;
master master ;

/******************************************************************************
 * Signal blocking/unblocking
 */

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

/******************************************************************************
 * Main function
 *
 * - check arguments
 * - read/parse configuration file
 * - start master: start threads for SOS engine, HTTP servers, etc.
 * - wait for a signal and stop all master threads
 */

int main (int argc, char *argv [])
{
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

	if (! cf.parse (conffile))
	{
	    std::cerr << "Read error for " << conffile << "\n" ;
	    std::cerr << "Abort\n" ;
	    exit (1) ;
	}

	std::cout << cf ;

	block_all_signals (&mask) ;

	if (! master.start (cf))
	{
	    std::cerr << "Cannot start\n" ;
	    exit (1) ;
	}

	undo_block_all_signals (&mask) ;
	wait_for_signal () ;

	if (! master.stop ())
	{
	    std::cerr << "Cannot stop\n" ;
	    exit (1) ;
	}
    }
    catch (std::exception &e)
    {
	std::cerr << "Exception: " << e.what () << "\n" ;
    }

    exit (0) ;
}
