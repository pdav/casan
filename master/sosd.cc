#include <iostream>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "l2.h"
#include "l2eth.h"
#include "engine.h"

int main (int argc, char *argv [])
{
    l2net *l ;
    slave s ;				// slave
    engine e ;

    // start SOS engine machinery
    e.ttl (DEFAULT_SLAVE_TTL) ;
    e.init () ;

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
}
