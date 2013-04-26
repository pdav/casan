#ifndef SOS_L2ETH_H
#define	SOS_L2ETH_H

#include <iostream>

#include "sos.h"
#include "l2.h"

class l2net_eth ;

class l2addr_eth: public l2addr
{
    public:
	l2addr_eth () ;				// constructor
	l2addr_eth (const char *) ;		// constructor
	l2addr_eth (const l2addr_eth &) ;	// copy constructor
	l2addr_eth &operator= (const l2addr_eth &) ; // copy assignment

	~l2addr_eth () ;			// destructor

	int operator== (const l2addr &) ;
	int operator!= (const l2addr &) ;

	friend class l2net_eth ;

	// friend ostream &operator << (ostream &o, const l2addr_eth &addr) ;
} ;

extern l2addr_eth l2addr_eth_broadcast ;

class l2net_eth: public l2net
{
    public:
	int init (const char *iface) ;
	void term (void) ;
	int send (l2addr *daddr, void *data, int len) ;
	int bsend (void *data, int len) ;
	pktype_t recv (l2addr **saddr, void *data, int *len) ;
	l2addr *bcastaddr (void) ;

    private:
	int ifidx_ ;			// interface index
} ;

#endif
