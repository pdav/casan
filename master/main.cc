#include <iostream>

#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define	NTAB(t)		(sizeof (t)/sizeof (t)[0])

#include "l2.h"
#include "l2eth.h"
#include "engine.h"

int data_available (int desc)
{
    struct pollfd fds [1] ;
    int r ;

    fds [0].fd = desc ;
    fds [0].events = POLLIN ;
    fds [0].revents = 0 ;
    r = poll (fds, NTAB (fds), 0) ;
    return r == 1 ;
}

int main (int argc, char *argv [])
{
    l2net *l ;
    l2net_eth *le ;
    l2addr_eth *ae ;
    l2addr_eth rae ;
    l2addr *la ;
    int s ;
    byte buf [512] ;
    char coucou [] = "coucou++ !\r" ;
    engine e ;

    e.init () ;
    std::cout << "JE M'ENDORS\n" ;
    for (;;)
    {
	sleep (3) ;
	le = new l2net_eth ;
	e.start_net (le) ;
    }

#if 0
    le = new l2net_eth ;
    l = le ;

    ae = new l2addr_eth ("ff:ff:ff:ff:ff:ff") ;

    if (l->init ("eth0") == -1)
    {
	perror ("init") ;
	exit (1) ;
    }

    for (;;)
    {
	if (l->send (ae, buf, sizeof buf) == -1)
	    perror ("send") ;

	if (data_available (l->getfd ()))
	{
	    int len ;
	    pktype_t pkt ;

	    len = sizeof (buf) ;
	    pkt = l->recv (&rae, buf, &len) ;
	    std::cout << "pkt=" << pkt << ", len=" << len << std::endl ;
	}

	sleep (1) ;
    }
#endif
}
