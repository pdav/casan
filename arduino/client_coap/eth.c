#include <sys/types.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>

#include "sos.h"
#include "net.h"
#include "eth.h"

// http://stackoverflow.com/questions/3366812/linux-raw-ethernet-socket-bind-to-specific-protocol
#define	ETHTYPE_SOS	0x4242 //0x88b5		// public use for prototype
// #define	ETHTYPE_SOS	0x0806		// ARP

l2desc_t *eth_init (const char *iface)
{
	sos_eth_t *s ;
	struct sockaddr_ll sll ;
	int sd, ifidx ;

	/* Get interface index */

	ifidx = if_nametoindex (iface) ;
	if (ifidx == 0)
		return NULL ;

	/* Open socket */

	sd = socket (PF_PACKET, SOCK_DGRAM, htons (ETHTYPE_SOS)) ;
	if (sd == -1)
		return NULL ;


	/* Bind the socket to the interface */

	sll.sll_family = AF_PACKET ;
	sll.sll_protocol = htons (ETHTYPE_SOS) ;
	sll.sll_ifindex = ifidx ;
	if (bind (sd, (struct sockaddr *) &sll, sizeof sll) == -1)
	{
		close (sd) ;
		return NULL ;
	}

	/* pack everything in the descriptor */

	s = malloc (sizeof *s) ;
	if (s == NULL)
	{
		close (sd) ;
		return NULL ;
	}

	s->sd = sd ;
	s->ifidx = ifidx ;

	return (l2desc_t *) s ;
}

void eth_close (l2desc_t *desc)
{
	sos_eth_t *s = (sos_eth_t *) desc ;

	close (s->sd) ;
	free (s) ;
}

int eth_getfd (l2desc_t *desc)
{
	sos_eth_t *s = (sos_eth_t *) desc ;
	return s->sd ;
}

int eth_send (l2desc_t *desc, ethaddr_t daddr, void *data, int len)
{
	sos_eth_t *s = (sos_eth_t *) desc ;
	struct sockaddr_ll sll ;
	int r ;

	memset (&sll, 0, sizeof sll) ;
	sll.sll_family = AF_PACKET ;
	sll.sll_protocol = htons (ETHTYPE_SOS) ;
	sll.sll_halen = ETHADDRLEN ;
	sll.sll_ifindex = s->ifidx ;
	memcpy (sll.sll_addr, daddr, ETHADDRLEN) ;

	r = sendto (s->sd, data, len, 0, (struct sockaddr *) &sll, sizeof sll) ;
	return r ;
}

int eth_is_data_available (l2desc_t *desc)
{
	sos_eth_t *s = (sos_eth_t *) desc ;
	struct pollfd fds [1] ;
	int r ;

	fds [0].fd = s->sd ;
	fds [0].events = POLLIN ;
	fds [0].revents = 0 ;
	r = poll (fds, NTAB (fds), 0) ;
	return r == 1 ;
}

pktype_t eth_recv (l2desc_t *desc, ethaddr_t saddr, void *data, int *len)
{
	sos_eth_t *s = (sos_eth_t *) desc ;
	struct sockaddr_ll sll ;
	int ssll ;
	int r ;
	pktype_t pktype ;

	memset (&sll, 0, sizeof sll) ;
	sll.sll_family = AF_PACKET ;
	sll.sll_ifindex = s->ifidx ;
	sll.sll_protocol = htons (ETHTYPE_SOS) ;
	sll.sll_halen = ETHADDRLEN ;

	ssll = sizeof sll ;

	r = recvfrom (s->sd, data, *len, 0, (struct sockaddr *) &sll, &ssll) ;
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

		memcpy (saddr, sll.sll_addr, ETHADDRLEN) ;
	}

	return pktype ;
}

#ifdef VERSION0
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
		// On rajoute le type
		char a_envoyer[10];
		// Type 1 | TKL 4
		a_envoyer[0] = 0x14;
		a_envoyer[1] = 0x23;

		// ID : 1
		a_envoyer[2] = 0x00;
		a_envoyer[3] = 0x01;

		// TK
		a_envoyer[4] = 0x11;
		a_envoyer[5] = 0x12;
		a_envoyer[6] = 0x13;
		a_envoyer[7] = 0x14;

		// Séparateur
		a_envoyer[8] = 0xFF;
	
		// données
		a_envoyer[9] = 0x11;
		a_envoyer[10] = 0x12;
		a_envoyer[11] = '\0';

		ethaddr_t b = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } ;
		int r ;

		r = eth_send (desc, b, a_envoyer, 12) ;
		if (r == -1)
			perror ("eth_send") ;

		printf("On a envoyé un truc\n");
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
