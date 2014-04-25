/*
 * Test program for the "l2net_*" class
 */

#include "sos.h"

#ifdef L2_ETH
    #include "l2-eth.h"

    l2addr *myaddr = new l2addr_eth ("00:01:02:03:04:05") ;
    l2addr *destaddr = new l2addr_eth ("e8:e0:b7:29:03:63") ;

    l2net_eth l2 ;

    // MTU is less than 0.25 * (free memory in SRAM after initialization)
    #define	MTU		200
#endif
#ifdef L2_154
    #include "l2-154.h"

    l2addr *myaddr = new l2addr_154 ("45:67") ;
    l2addr *destaddr = new l2addr_154 ("12:34") ;

    l2net_154 l2 ;

    #define	CHANNEL		25
    #define	PANID		CONST16 (0xca, 0xfe)
    #define	MTU		0
#endif

#define	DEBUGINTERVAL	15

Debug debug ;

void setup ()
{
    Serial.begin (38400) ;
    debug.start (DEBUGINTERVAL) ;
#ifdef L2_ETH
    l2.start (myaddr, false, MTU, ETHTYPE) ;
#endif
#ifdef L2_154
    l2.start (myaddr, false, MTU, CHANNEL, PANID) ;
#endif
}

void print_paylen_src_dst (void)
{
    l2addr *src, *dst ;

    Serial.print (F (" payload length=")) ;
    Serial.print (l2.get_paylen ()) ;
    Serial.print (F (" ")) ;
    src = l2.get_src () ; src->print () ;
    Serial.print (F ("->")) ;
    dst = l2.get_dst () ; dst->print () ;
    Serial.println () ;
}

void recv_l2 (void)
{
    l2net::l2_recv_t r ;

    r = l2.recv () ;

    switch (r)
    {
	case l2net::RECV_EMPTY :
	    break ;
	case l2net::RECV_OK :
	    Serial.print (F ("OK")) ;
	    print_paylen_src_dst () ;
	    l2.dump_packet (0, 20) ;
	    break ;
	case l2net::RECV_WRONG_DEST :
	    Serial.print (F ("WRONG DEST")) ;
	    print_paylen_src_dst () ;
	    l2.dump_packet (0, 20) ;
	    break ;
	case l2net::RECV_WRONG_TYPE :
	    Serial.print (F ("WRONG TYPE")) ;
	    print_paylen_src_dst () ;
	    l2.dump_packet (0, 20) ;
	    break ;
	case l2net::RECV_TRUNCATED :
	    Serial.print (F ("TRUNCATED")) ;
	    print_paylen_src_dst () ;
	    break ;
	default :
	    Serial.print (F ("UNKNOWN : r=")) ;
	    Serial.print (r) ;
	    Serial.print (F (", payload length=")) ;
	    Serial.print (l2.get_paylen ()) ;
	    Serial.println () ;
	    break ;
    }
}

char testpkt [] = "This is a very long test packet (more than 46 bytes) to send to a broadcast address" ;

void send_l2 (void)
{
    bool r ;

    r = l2.send (* l2.bcastaddr (), (uint8_t *) testpkt, sizeof testpkt - 1) ;
    Serial.print (F ("Sent broacast: r=")) ;
    Serial.print (r) ;
    Serial.println () ;

    r = l2.send (*destaddr, (uint8_t *) testpkt, sizeof testpkt - 1) ;
    Serial.print (F ("Sent unicast: r=")) ;
    Serial.print (r) ;
    Serial.println () ;
}

void loop ()
{
    if (debug.heartbeat ())
	send_l2 () ;
    recv_l2 () ;
}
