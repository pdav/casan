/*
 * Test program for the "l2net_*" class
 */

#include "sos.h"

#ifdef L2_ETH
    #include "l2-eth.h"

    l2addr *myaddr = new l2addr_eth ("00:01:02:03:04:05") ;
    l2addr *destaddr = new l2addr_eth ("ff:ff:ff:ff:ff:ff") ;

    l2net_eth l2 ;

    // MTU is less than 0.25 * (free memory in SRAM after initialization)
    #define	MTU		200
#endif
#ifdef L2_154
    #include "l2-154.h"

    l2addr *myaddr = new l2addr_154 ("45:67") ;
    l2addr *destaddr = new l2addr_154 ("ff:ff") ;

    l2net_154 l2 ;

    #define	CHANNEL		25
    #define	PANID		CONST16 (0xca, 0xfe)
    #define	MTU		0
#endif

#define	DEBUGINTERVAL	15

void setup ()
{
    Serial.begin (38400) ;
    debug.start (DEBUGINTERVAL) ;
#ifdef L2_ETH
    l2.start (myaddr, false, MTU, SOS_ETH_TYPE) ;
#endif
#ifdef L2_154
    l2.start (myaddr, false, MTU, CHANNEL, PANID) ;
#endif
}

void recv_l2 (void)
{
    l2_recv_t r ;
    l2addr *src, *dst ;
    int paylen ;

    r = l2.recv () ;
    paylen = l2.get_paylen () ;
    src = l2.get_src () ;
    dst = l2.get_dst () ;
    switch (r)
    {
	case L2_RECV_EMPTY :
	    break ;
	case L2_RECV_RECV_OK :
	    Serial.print (F ("OK : payload length=")) ;
	    Serial.print (paylen) ;
	    Serial.println () ;
	    l2.dump_packet (0, 20) ;
	    break ;
	case L2_RECV_WRONG_DEST :
	    Serial.print (F ("WRONG DEST : payload length=")) ;
	    Serial.print (paylen) ;
	    Serial.println () ;
	    l2.dump_packet (0, 20) ;
	    break ;
	case L2_RECV_WRONG_ETHTYPE :
	    Serial.print (F ("WRONG ETHTYPE : payload length=")) ;
	    Serial.print (paylen) ;
	    Serial.println () ;
	    l2.dump_packet (0, 20) ;
	    break ;
	case L2_RECV_TRUNCATED :
	    Serial.print (F ("TRUNCATED : payload length=")) ;
	    Serial.print (paylen) ;
	    Serial.println () ;
	    break ;
	default :
	    Serial.print (F ("UNKNOWN : r=")) ;
	    Serial.print (r) ;
	    Serial.print (F (", payload length=")) ;
	    Serial.print (paylen) ;
	    Serial.println () ;
	    break ;
    }
}

char testpkt [] = "This is a very long test packet (more than 46 bytes) to send to a broadcast address" ;

void send_l2 (void)
{
    bool r ;

    r = l2.send (*destaddr, (uint8_t *) testpkt, sizeof testpkt) ;
    Serial.print (F ("Sent: r=")) ;
    Serial.print (r) ;
    Serial.println () ;
}


void loop ()
{
    if (debug.heartbeat ())
	send_l2 () ;

    recv_l2 () ;
}
