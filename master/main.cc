#include <iostream>

#include <poll.h>
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
    l2net_eth *le ;
    l2addr_eth *ae ;
    l2addr_eth rae ;
    l2addr *la ;
    l2addr_eth *sa ;			// slave address
    slave s ;				// slave
    byte buf [512] ;
    char coucou [] = "coucou++ !\r" ;
    engine e ;

    // start SOS engine machinery
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

    sa = new l2addr_eth ("90:a2:da:80:0a:d4") ;
    s.set_l2addr (sa) ;
    s.set_l2net (l) ;
    e.add_slave (s) ;

    sleep (1000) ;

}
