#include <iostream>
#include <cstring>
#include <vector>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "global.h"

#include "l2.h"
#include "l2802154.h"
#include "msg.h"
#include "resource.h"
#include "sos.h"

#define IFACE		"ttyUSB0"
// #define ADDR		"90:a2:da:80:0a:d4"		// arduino
#define SADDR		"ca:fe"
#define	PANID		"56:78"
#define DADDR		"12:34"
#define	CHANNEL		25

#define	PATH_1		".well-known"
#define	PATH_2		"sos"

#define	SLAVE169	"slave=169"
#define	MTU		"mtu=100"

int main (int argc, char *argv [])
{
    sos::l2net *l ;
    sos::l2net_802154 *le ;
    sos::l2addr_802154 *sa ;		// slave address
    sos::slave s ;			// slave
    sos::slave sb ;			// pseudo-slave for broadcast
    sos::msg m1, m2 ;
    char buf [512] = "" ;
    std::string iface = "ttyUSB0" ;
    std::string myaddr = SADDR ;
    std::string panid = PANID ;

    // start new interface
    le = new sos::l2net_802154 ;
    if (le->init (iface, "xbee", myaddr, panid, CHANNEL) == -1)
    {
	perror ("init") ;
	exit (1) ;
    }

    std::cout << IFACE << " initialized for " << myaddr << "/" << PANID
			<< " on channel " << CHANNEL << "\n" ;
    // from now on, use only generic l2net interface
    l = le ;

    // register new slave
    sa = new sos::l2addr_802154 (DADDR) ;
    s.addr (sa) ;
    s.l2 (l) ;

    // pseudo-slave for broadcast address
    sb.addr (&sos::l2addr_802154_broadcast) ;
    sb.l2 (l) ;

    // sleep (1) ;

    sos::option uri_path1 (sos::option::MO_Uri_Path, (void *) PATH_1, sizeof PATH_1 - 1) ;
    // sos::option uri_path2 (sos::option::MO_Uri_Path, (void *) PATH_2, sizeof PATH_2 - 1) ;
    sos::option uri_path2 ;

    uri_path2 = uri_path1 ;
    uri_path2.optval ((void *) PATH_2, sizeof PATH_2 - 1) ;

    // DISCOVER message
    m1.type (sos::msg::MT_NON) ;
    m1.code (sos::msg::MC_POST) ;
    m1.peer (&sb) ;
    m1.id (10) ;
    m1.pushoption (uri_path1) ;
    m1.pushoption (uri_path2) ;
    sos::option os (sos::option::MO_Uri_Query, (void *) SLAVE169, sizeof SLAVE169 - 1) ;
    m1.pushoption (os) ;
    sos::option os2 (sos::option::MO_Uri_Query, (void *) MTU, sizeof MTU - 1) ;
    m1.pushoption (os2) ;
    m1.send () ;

    sleep (2) ;

    // ASSOCIATE answer message
    std::strcpy (buf, "</test>;title=\"My test\";ct=0,</foo>;if=\"clock\";rt=\"Ticks;title=\"My clock\";ct=0;obs") ;
    m2.type (sos::msg::MT_ACK) ;		// will not be retransmitted
    m2.code (COAP_MKCODE (2, 5)) ;
    m2.peer (&s) ;
    m2.payload (buf, strlen (buf)) ;
    m2.id (2) ;
    sos::option ocf (sos::option::MO_Content_Format, (void *) "abc", sizeof "abc" - 1) ;
    m2.pushoption (ocf) ;
    m2.send () ;

    delete sa ;
    delete l ;
}
