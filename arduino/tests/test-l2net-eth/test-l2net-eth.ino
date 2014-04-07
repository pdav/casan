/*
 * Test program for the "l2addr_eth" class
 */

#include <SPI.h>
#include "sos.h"
#include "l2eth.h"

l2addr *dest_addr = new l2addr_eth ("ff:ff:ff:ff:ff:ff") ;
l2addr *my_addr = new l2addr_eth ("00:01:02:03:04:05") ;
l2net_eth e ;

bool promisc = false ;
int mtu = 500 ;


void setup ()
{
    Serial.begin(38400) ;
    Serial.println (F ("start")) ;
    e.start (my_addr, promisc, mtu, SOS_ETH_TYPE) ;
}

void recv_eth (void)
{
    l2_recv_t r ;
    l2addr_eth src, dst ;
    int paylen ;

    r = e.recv () ;
    paylen = e.get_paylen () ;
    e.get_src (&src) ;
    e.get_dst (&dst) ;
    switch (r)
    {
	case L2_RECV_EMPTY :
	    break ;
	case L2_RECV_RECV_OK :
	    Serial.print (F ("OK : payload length=")) ;
	    Serial.print (paylen) ;
	    Serial.println () ;
	    e.dump_packet (14, 20) ;
	    break ;
	case L2_RECV_WRONG_DEST :
	    Serial.print (F ("WRONG DEST : payload length=")) ;
	    Serial.print (paylen) ;
	    Serial.println () ;
	    e.dump_packet (0, 20) ;
	    break ;
	case L2_RECV_WRONG_ETHTYPE :
	    Serial.print (F ("WRONG ETHTYPE : payload length=")) ;
	    Serial.print (paylen) ;
	    Serial.println () ;
	    e.dump_packet (0, 20) ;
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

char testpkt [] = "This is a test packet to broadcast address" ;

void send_eth (void)
{
    size_t r ;

    r = e.send (*dest_addr, (uint8_t *) testpkt, sizeof testpkt) ;
    Serial.print (F ("Sent: r=")) ;
    Serial.print (r) ;
    Serial.println () ;
}


long int n ;

void loop ()
{
    if (n++ % 100000 == 0)
    {
	PRINT_DEBUG_STATIC ("\033[36m\tloop \033[00m ") ;
	PRINT_FREE_MEM ;		// check memory leak
	send_eth () ;
	n = 1 ;
    }

    recv_eth () ;
}
