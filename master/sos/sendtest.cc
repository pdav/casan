#include <iostream>
#include <cstring>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "global.h"

#include "l2.h"
#include "l2eth.h"
#include "msg.h"
#include "sos.h"

#define IFACE		"eth0"
// #define ADDR		"90:a2:da:80:0a:d4"		// arduino
#define ADDR		"e8:e0:b7:29:03:63"		// vagabond
// #define ADDR		"52:54:00:f5:7b:46"		// lognet

#define	PATH_1		".well-known"
#define	PATH_2		"sos"

#define	SLAVE169	"slave=169"

int main (int argc, char *argv [])
{
    l2net *l ;
    l2addr_eth *sa ;			// slave address
    slave s ;				// slave
    slave sb ;				// pseudo-slave for broadcast
    msg m1, m2 ;
    char buf [512] = "" ;

    // start new interface
    l = new l2net_eth ;
    if (l->init (IFACE) == -1)
    {
	perror ("init") ;
	exit (1) ;
    }

    // register new slave
    sa = new l2addr_eth (ADDR) ;
    s.addr (sa) ;
    s.l2 (l) ;

    // pseudo-slave for broadcast address
    sb.addr (&l2addr_eth_broadcast) ;
    sb.l2 (l) ;

    std::cout << IFACE << " initialized for " << ADDR << "\n" ;

    // sleep (1) ;

    option uri_path1 (option::MO_Uri_Path, (void *) PATH_1, sizeof PATH_1 - 1) ;
    option uri_path2 (option::MO_Uri_Path, (void *) PATH_2, sizeof PATH_2 - 1) ;

    // REGISTER message
    m1.type (msg::MT_NON) ;
    m1.code (msg::MC_POST) ;
    m1.peer (&sb) ;
    m1.id (10) ;
    m1.pushoption (uri_path1) ;
    m1.pushoption (uri_path2) ;
    option os (option::MO_Uri_Query, (void *) SLAVE169, sizeof SLAVE169 - 1) ;
    m1.pushoption (os) ;
    m1.send () ;

    sleep (2) ;

    // ASSOCIATE answer message
    std::strcpy (buf, "<resource list>") ;
    m2.type (msg::MT_ACK) ;		// will not be retransmitted
    m2.code (COAP_MKCODE (2, 5)) ;
    m2.peer (&s) ;
    m2.payload (buf, strlen (buf)) ;
    m2.id (3) ;
    option ocf (option::MO_Content_Format, (void *) "abc", sizeof "abc" - 1) ;
    m2.pushoption (ocf) ;
    m2.send () ;
}
