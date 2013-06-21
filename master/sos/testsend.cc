#include <iostream>
#include <cstring>
#include <vector>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "global.h"

#include "l2.h"
#include "l2eth.h"
#include "msg.h"
#include "resource.h"
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
    sos::l2net *l ;
    sos::l2net_eth *le ;
    sos::l2addr_eth *sa ;		// slave address
    sos::slave s ;			// slave
    sos::slave sb ;			// pseudo-slave for broadcast
    sos::msg m1, m2 ;
    char buf [512] = "" ;

    // start new interface
    le = new sos::l2net_eth ;
    if (le->init (IFACE, ETHTYPE_SOS) == -1)
    {
	perror ("init") ;
	exit (1) ;
    }

    // from now on, use only generic l2net interface
    l = le ;

    // register new slave
    sa = new sos::l2addr_eth (ADDR) ;
    s.addr (sa) ;
    s.l2 (l) ;

    // pseudo-slave for broadcast address
    sb.addr (&sos::l2addr_eth_broadcast) ;
    sb.l2 (l) ;

    std::cout << IFACE << " initialized for " << ADDR << "\n" ;

    // sleep (1) ;

    sos::option uri_path1 (sos::option::MO_Uri_Path, (void *) PATH_1, sizeof PATH_1 - 1) ;
    // sos::option uri_path2 (sos::option::MO_Uri_Path, (void *) PATH_2, sizeof PATH_2 - 1) ;
    sos::option uri_path2 ;

    uri_path2 = uri_path1 ;
    uri_path2.optval ((void *) PATH_2, sizeof PATH_2 - 1) ;

    // REGISTER message
    m1.type (sos::msg::MT_NON) ;
    m1.code (sos::msg::MC_POST) ;
    m1.peer (&sb) ;
    m1.id (10) ;
    m1.pushoption (uri_path1) ;
    m1.pushoption (uri_path2) ;
    sos::option os (sos::option::MO_Uri_Query, (void *) SLAVE169, sizeof SLAVE169 - 1) ;
    m1.pushoption (os) ;
    m1.send () ;

    sleep (2) ;

    // ASSOCIATE answer message
    std::strcpy (buf, "</test>;title=\"My test\";ct=0,</foo>;if=\"clock\";rt=\"Ticks;title=\"My clock\";ct=0;obs") ;
    m2.type (sos::msg::MT_ACK) ;		// will not be retransmitted
    m2.code (COAP_MKCODE (2, 5)) ;
    m2.peer (&s) ;
    m2.payload (buf, strlen (buf)) ;
    m2.id (3) ;
    sos::option ocf (sos::option::MO_Content_Format, (void *) "abc", sizeof "abc" - 1) ;
    m2.pushoption (ocf) ;
    m2.send () ;

    delete sa ;
    delete l ;
}
