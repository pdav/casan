#include <iostream>

#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define	NTAB(t)		(sizeof (t)/sizeof (t)[0])

#include "l2.h"
#include "l2eth.h"
#include "engine.h"

#define	IFACE		"eth0"

int main (int argc, char *argv [])
{
    l2net *l ;
    // l2addr *la ;
    l2addr_eth *sa ;			// slave address
    slave s ;				// slave
    engine e ;
    msg m, m2 ;
    char buf [] = "coucou\n" ;

    // start SOS engine machinery
    e.init () ;

    // start new interface
    l = new l2net_eth ;
    if (l->init (IFACE) == -1)
    {
	perror ("init") ;
	exit (1) ;
    }
    // register network
    e.start_net (l) ;
    std::cout << IFACE << " initialized\n" ;

    // register new slave
    // sa = new l2addr_eth ("90:a2:da:80:0a:d4") ;	// arduino
    sa = new l2addr_eth ("e8:e0:b7:29:03:63") ;		// vagabond
    s.addr (sa) ;
    s.l2 (l) ;
    e.add_slave (&s) ;

    sleep (2) ;
    
    // register new message
    m.type (msg::MT_CON) ;		// will be retransmitted
    m.code (msg::MC_GET) ;
    m.peer (&s) ;
    m.token ((void *) "ABCD", 4) ;
    m.payload (buf, sizeof buf) ;
    e.add_request (&m) ;

    sleep (5) ;

    // register another NON message
    m2 = m ;				// basically the same as before
    m2.type (msg::MT_NON) ;		// will not be retransmitted
    m2.id (0) ;				// id = 0 => let SOS choose it
    e.add_request (&m2) ;

    sleep (1000) ;

}
