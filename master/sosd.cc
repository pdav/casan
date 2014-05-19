/**
 * @file sosd.cpp
 * @brief SOS main program
 *
 * This file contains the `main` function of the SOS master.
 * It reads the configuration file, then calls the master to
 * start the various threads (HTTP server, SOS listener, etc.)
 * and waits for a signal to cleanly exit.
 */

#include <iostream>
#include <string>
#include <list>

#include <unistd.h>
#include <signal.h>

#include "server.hpp"

#include "master.h"

#define	CONFFILE	"./sosd.conf"

conf cf ;
master master ;
int debug_levels ;

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
 * Debug facilities
 */

struct
{
    int level ;
    const char *title ;
} debug_names [] = 
{
    {D_MESSAGE,	"message"}, 
    {D_OPTION,	"option"}, 
    {D_STATE,	"state"}, 
    {D_CACHE,	"cache"}, 
    {D_CONF,	"conf"}, 
    {D_HTTP,	"http"}, 
    {(1<<30)-1, "all"},
} ;

const char *debug_title (int level)
{
    const char *r = "(unknown level)" ;

    for (int i = 0 ; i < NTAB (debug_names) ; i++)
    {
	if (debug_names [i].level == level)
	{
	    r = debug_names [i].title ;
	    break ;
	}
    }
    return r ;
}

int debug_index (const char *name)
{
    int r = -1 ;

    for (int i = 0 ; i < NTAB (debug_names) ; i++)
    {
	if (strcmp (name, debug_names [i].title) == 0)
	{
	    r = i ;
	    break ;
	}
    }
    return r ;
}

bool set_debug_levels (int &debuglevel, int sign, const char *name)
{
    int idx = debug_index (name) ;
    if (idx != -1)
    {
	if (sign > 0)
	    debuglevel |= debug_names [idx].level ;
	else
	    debuglevel &= ~debug_names [idx].level ;
    }
    return idx != -1 ;
}

bool update_debug_levels (int &debuglevel, char *optarg)
{
    int sign = 1 ;
    char *q = NULL ;
    bool r ;

    r = true ;
    for (auto p = optarg ; *p != '\0' ; p++)
    {
	if (*p == '+' || *p == ' ' || *p == '-')
	{
	    int newsign = (*p == '-') ? -1 : 1 ;
	    if (q != NULL)
	    {
		*p = '\0' ;
		if (! set_debug_levels (debuglevel, sign, q))
		{
		    r = false ;
		    break ;
		}
		q = NULL ;
		sign = newsign ;
	    }
	    else
	    {
		sign *= newsign ;
	    }
	}
	else if (isalpha (*p))
	{
	    if (q == NULL)
		q = p ;
	}
	else
	{
	    r = false ;
	    break ;
	}
    }
    if (r && q != NULL)
	if (! set_debug_levels (debuglevel, sign, q))
	    r = false ;
    return r ;
}

void usage (const char *prog)
{
    for (auto p = prog ; *p != '\0' ; p++)
    {
	if (*p == '/')
	    prog = p + 1 ;
    }

    std::cerr << "usage: " << prog << " [-h][-d debug-spec][-c conf-file]\n" ;
    std::cerr << "\tdebug-spec::= [+|-]spec[+|-]spec... \n" ;
    std::cerr << "\tspec: " ;
    for (int i = 0 ; i < NTAB (debug_names) ; i++)
    {
	if (i > 0)
	    std::cerr << ", " ;
	std::cerr << debug_names [i].title ;
    }
    std::cerr << "\n" ;
    std::cerr << "Example: " << prog << " -d all-option -c ./sosd.conf\n" ;
}

/******************************************************************************
 */

/**
 * @brief Main function
 *
 * The `main` function:
 * - check arguments
 * - read/parse configuration file
 * - start master: start threads for SOS engine, HTTP servers, etc.
 * - wait for a signal and stop all master threads
 */

int main (int argc, char *argv [])
{
    const char *conffile ;
    sigset_t mask ;

    debug_levels = 0 ;			// Default: no debug message
    conffile = CONFFILE ;

    try
    {
	int opt ;

	while ((opt = getopt (argc, argv, "hd:c:")) != -1)
	{
	    switch (opt)
	    {
		case 'h' :
		    usage (argv [0]) ;
		    exit (0) ;
		    break ;
		case 'd' :
		    if (! update_debug_levels (debug_levels, optarg))
		    {
			usage (argv [0]) ;
			exit (1) ;
		    }
		    break ;
		case 'c' :
		    conffile = optarg ;
		    break ;
		default :
		    usage (argv [0]) ;
		    exit (1) ;
	    }
	}
	if (argc > optind)
	{
	    usage (argv [0]) ;
	    exit (1) ;
	}

	if (! cf.parse (conffile))
	{
	    std::cerr << "Read error for " << conffile << "\n" ;
	    std::cerr << "Abort\n" ;
	    exit (1) ;
	}

	D (D_CONF, "read configuration:\n" << cf) ;

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
