#include <iostream>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define	NTAB(t)		(sizeof (t)/sizeof (t)[0])

#include "l2.h"
#include "l2eth.h"
#include "engine.h"

int main (int argc, char *argv [])
{
    l2net *l ;
    slave s ;				// slave
    engine e ;
    msg m, m2 ;
    char buf [] = "coucou\n" ;

    // start SOS engine machinery
    e.init () ;
    e.ttl (DEFAULT_SLAVE_TTL) ;

    // start new interface
    l = new l2net_eth ;
    if (l->init ("eth0") == -1)
    {
	perror ("init") ;
	exit (1) ;
    }
    // register network
    e.start_net (l) ;
    std::cout << "eth0 initialized\n" ;

    s.slaveid (169) ;			// no l2 nor addr specified
    e.add_slave (&s) ;

    sleep (100000) ;
    sleep (10) ;
    
    // register new message
    m.type (msg::MT_CON) ;
    m.code (msg::MC_GET) ;
    m.peer (&s) ;
    m.token ((void *) "ABCD", 4) ;
    m.payload (buf, sizeof buf) ;
    e.add_request (&m) ;

    sleep (10) ;

    // register another NON message
    m2 = m ;
    m2.type (msg::MT_NON) ;
    m2.id (0) ;
    e.add_request (&m2) ;

    sleep (1000) ;
}
