/*
 * Test program for sending and receiving SOS/CoAP messages
 */

#include "sos.h"

#define	PATH1		".well-known"
#define	PATH2		"sos"
#define	PATH3		"path3"

#define	DEBUGINTERVAL	5

// #if L2_ETH
#include "l2-eth.h"

l2addr *myaddr = new l2addr_eth ("00:01:02:03:04:05") ;
l2addr *dest = & l2addr_eth_broadcast ;
l2net_eth e ;

bool promisc = false ;
int mtu = 200 ;
// #endif

Debug debug ;

void setup ()
{
    Serial.begin (38400) ;
    e.start (myaddr, promisc, mtu, SOS_ETH_TYPE) ;
    debug.start (DEBUGINTERVAL) ;
}

void test_recv (void)
{
    Msg in ;
    l2net::l2_recv_t r ;

    while ((r = in.recv (e)) == l2net::RECV_OK)
	in.print () ;
}

void res_send (int msgnum, bool ok)
{
    Serial.print (F ("Sending message ")) ;
    Serial.print (msgnum) ;
    Serial.print (F (": ")) ;
    Serial.print (ok) ;
    Serial.println () ;
}

void test_send (void)
{
    Msg m1 ;
    Msg m2 ;
    bool ok ;

    option up1 (option::MO_Uri_Path, PATH1, sizeof PATH1 - 1) ;
    option up2 (option::MO_Uri_Path, PATH2, sizeof PATH2 - 1) ;
    option up3 (option::MO_Uri_Path, PATH3, sizeof PATH3 - 1) ;
    option ocf (option::MO_Content_Format, "abc", sizeof "abc" - 1) ;

    m1.set_id (258) ;
    m1.set_type (COAP_TYPE_NON) ;
    m1.push_option (ocf) ;
    m1.push_option (up1) ;
    m1.push_option (up2) ;
    m1.push_option (up3) ;
    ok = m1.send (e, *dest) ;
    res_send (1, ok) ;

    m2.set_id (33) ;
    m2.set_type (COAP_TYPE_CON) ;
    m2.push_option (ocf) ;
    ok = m2.send (e, *dest) ;
    res_send (2, ok) ;
}

void loop ()
{
    if (debug.heartbeat ())
	test_send () ;
    test_recv () ;
}
