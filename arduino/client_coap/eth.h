#ifndef SOS_ETH_H
#define	SOS_ETH_H

#define	ETHADDRLEN	6

typedef unsigned char ethaddr_t [ETHADDRLEN] ;

struct sos_eth
{
	int sd ;				// socket descriptor
	int ifidx ;				// interface index
} ;
typedef struct sos_eth sos_eth_t ;

l2desc_t *eth_init (const char *iface) ;
void eth_close (l2desc_t *desc) ;
int eth_getfd (l2desc_t *desc) ;
int eth_send (l2desc_t *desc, ethaddr_t daddr, void *data, int len) ;
pktype_t eth_recv (l2desc_t *desc, ethaddr_t saddr, void *data, int *len) ;

#endif
