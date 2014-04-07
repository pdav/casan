#ifndef	L2_H
#define	L2_H

/*
 * These classes are designed to virtualize specific L2 networks
 */

#include "defs.h"
#include "enum.h"
#include "memory_free.h"

class l2addr
{
    public:
	virtual ~l2addr () {} ;
	virtual bool operator== (const l2addr &other) = 0 ;
	virtual bool operator!= (const l2addr &other) = 0 ;
	virtual bool operator!= (const unsigned char* mac_addr) = 0 ;
} ;

class l2net {
    public:
	virtual ~l2net () {} ;
	virtual size_t send (l2addr &mac_dest, const uint8_t *data, size_t len) = 0 ;
	// the "recv" method copies the received packet in
	// the instance private variable (see _rbuf/_rbuflen below)
	virtual l2_recv_t recv (void) = 0 ;

	virtual l2addr *bcastaddr (void) = 0 ;
	virtual void get_src (l2addr *mac) = 0 ;
	virtual void get_dst (l2addr *mac) = 0 ;
	virtual uint8_t *get_payload (int offset) = 0 ;
	virtual size_t get_paylen (void) = 0 ;

	size_t mtu (void) { return mtu_ ; } ;

    protected:
	size_t mtu_ ;		// must be initialized in derived classes
} ;

#endif
