#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <netpacket/packet.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "l2.h"
#include "l2eth.h"

#define	ETHADDRLEN	6

// http://stackoverflow.com/questions/3366812/linux-raw-ethernet-socket-bind-to-specific-protocol
#define	ETHTYPE_SOS	0x88b5		// public use for prototype


/******************************************************************************
 * l2addr_eth methods
 */

// constructor
l2addr_eth::l2addr_eth ()
{
    addr = new byte [ETHADDRLEN] ;
}

// constructor
l2addr_eth::l2addr_eth (const char *a)
{
    int i = 0 ;
    byte b = 0 ;

    addr = new byte [ETHADDRLEN] ;

    while (*a != '\0' && i < ETHADDRLEN)
    {
	if (*a == ':')
	{
	    addr [i++] = b ;
	    b = 0 ;
	}
	else if (isxdigit (*a))
	{
	    byte x ;
	    char c ;

	    c = tolower (*a) ;
	    x = isdigit (c) ? (c - '0') : (c - 'a' + 10) ;
	    b = (b << 4) + x ;
	}
	else
	{
	    for (i = 0 ; i < ETHADDRLEN ; i++)
		addr [i] = 0 ;
	    break ;
	}
	a++ ;
    }
    if (i < ETHADDRLEN)
	addr [i] = b ;
}

// copy constructor
l2addr_eth::l2addr_eth (const l2addr_eth &x)
{
    addr = new byte [ETHADDRLEN] ;
    memcpy (addr, x.addr, ETHADDRLEN) ;
}

// assignment operator
l2addr_eth & l2addr_eth::operator = (const l2addr_eth &x)
{
    if (addr == x.addr)
	return *this ;

    if (addr == NULL)
	addr = new byte [ETHADDRLEN] ;
    memcpy (addr, x.addr, ETHADDRLEN) ;
    return *this ;
}

// destructor
l2addr_eth::~l2addr_eth ()
{
    delete [] addr ;
}

/******************************************************************************
 * l2net_eth methods
 */

int l2net_eth::init (const char *iface)
{
    struct sockaddr_ll sll ;

    /* Get interface index */

    ifidx = if_nametoindex (iface) ;
    if (ifidx == 0)
	return -1 ;

    /* Open socket */

    fd = socket (PF_PACKET, SOCK_DGRAM, htons (ETHTYPE_SOS)) ;
    if (fd == -1)
	return -1 ;


    /* Bind the socket to the interface */

    sll.sll_family = AF_PACKET ;
    sll.sll_protocol = htons (ETHTYPE_SOS) ;
    sll.sll_ifindex = ifidx ;
    if (bind (fd, (struct sockaddr *) &sll, sizeof sll) == -1)
    {
	close (fd) ;
	return -1 ;
    }

    return 0 ;
}

void l2net_eth::term (void)
{
    close (fd) ;
}

int l2net_eth::send (l2addr *daddr, void *data, int len)
{
    struct sockaddr_ll sll ;
    l2addr_eth *a = (l2addr_eth *) daddr ;
    int r ;

    memset (&sll, 0, sizeof sll) ;
    sll.sll_family = AF_PACKET ;
    sll.sll_protocol = htons (ETHTYPE_SOS) ;
    sll.sll_halen = ETHADDRLEN ;
    sll.sll_ifindex = ifidx ;
    memcpy (sll.sll_addr, a->addr, ETHADDRLEN) ;

    r = sendto (fd, data, len, 0, (struct sockaddr *) &sll, sizeof sll) ;
    return r ;
}

pktype_t l2net_eth::recv (l2addr *saddr, void *data, int *len)
{
    struct sockaddr_ll sll ;
    socklen_t ssll ;
    l2addr_eth *a = (l2addr_eth *) saddr ;
    int r ;
    pktype_t pktype ;

    memset (&sll, 0, sizeof sll) ;
    sll.sll_family = AF_PACKET ;
    sll.sll_ifindex = ifidx ;
    sll.sll_protocol = htons (ETHTYPE_SOS) ;
    sll.sll_halen = ETHADDRLEN ;

    ssll = sizeof sll ;

    r = recvfrom (fd, data, *len, 0, (struct sockaddr *) &sll, &ssll) ;
    if (r == -1)
    {
	pktype = PK_NONE ;
    }
    else
    {
	*len = r ;
	switch (sll.sll_pkttype)
	{
	    case PACKET_HOST :
		pktype = PK_ME ;
		break ;
	    case PACKET_BROADCAST :
	    case PACKET_MULTICAST :
		pktype = PK_BCAST ;
		break ;
	    case PACKET_OTHERHOST :
	    case PACKET_OUTGOING :
	    default :
		pktype = PK_NONE ;
		break ;
	}

	memcpy (a->addr, sll.sll_addr, ETHADDRLEN) ;
    }

    return pktype ;
}

#if 0
main (int argc, char *argv [])
{
    l2desc_t *desc ;

    if (argc != 2)
    {
	fprintf (stderr, "usage: %s if\n", argv [0]) ;
	exit (1) ;
    }

    desc = eth_init (argv [1]) ;
    if (desc == NULL)
    {
	perror ("open interface") ;
	exit (1) ;
    }

    for (;;)
    {
	char coucou [] = "coucou !\r" ;
	ethaddr_t b = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } ;
	int r ;

	r = eth_send (desc, b, coucou, sizeof coucou) ;
	if (r == -1)
	   perror ("eth_send") ;

	if (eth_is_data_available (desc))
	{
	    ethaddr_t saddr ;
	    pktype_t pkt ;
	    char data [2000] ;
	    int len ;

	    len = sizeof data ;
	    pkt = eth_recv (desc, saddr, data, &len) ;
	    printf ("pkt=%d len=%d\n", pkt, len) ;
	}
	sleep (1) ;
    }
}
#endif
