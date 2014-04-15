#ifndef L2_154_H
#define	L2_154_H

/*
 * These classes are used for IEEE 802.15.4 specialization
 */

#include <Arduino.h>
#include <ZigMsg.h>

#include "l2.h"
#include "defs.h"
#include "enum.h"

#define	I154ADDRLEN	2		// Address length
#define	I154MTU		127

class l2addr_154 : public l2addr
{
    public:
	l2addr_154 () ;					// constructor
	l2addr_154 (const char *) ;			// constructor
	l2addr_154 (const l2addr_154 &) ;		// copy constructor
	l2addr_154 &operator= (const l2addr_154 &) ;	// copy assignment

	bool operator== (const l2addr &) ;
	bool operator!= (const l2addr &) ;
	bool operator!= (const unsigned char *) ;	// all bytes
	void print (void) ;

    protected:
	addr2_t addr_ ;
	friend class l2net_154 ;
} ;

extern l2addr_154 l2addr_154_broadcast ;

/*
 * this class handles the lowest layer, it will talk directly with the 
 * arduino ethernet shield
 */
class l2net_154 : public l2net
{
    public:
	l2net_154 (void) ;
	~l2net_154 () ;
	void start (l2addr *myaddr, bool promisc, size_t mtu, channel_t chan, panid_t panid) ;
	bool send (l2addr &dest, const uint8_t *data, size_t len) ;
	// the "recv" method copy the received packet in
	// the instance private variable (see rbuf_/rbuflen_ below)
	l2_recv_t recv (void) ;

	l2addr *bcastaddr (void) ;	// return a static variable
	void get_src (l2addr *mac) ;	// fill an existing l2addr_154
	void get_dst (l2addr *mac) ;	// fill an existing l2addr_154
	l2addr *get_src (void) ;	// get a new l2addr_154
	l2addr *get_dst (void) ;	// get a new l2addr_154

	// Payload (not including MAC header, of course)
	uint8_t *get_payload (int offset) ;
	size_t get_paylen (void) ;	// if truncated pkt: truncated payload

	// debug usage
	void dump_packet (size_t start, size_t maxlen) ;

    private:
	ZigReceivedFrame *curframe_ ;
	addr2_t myaddr_ ;

	byte *rbuf_ ;			// receive buffer (of size mtu_ bytes)
	size_t pktlen_ ;		// real length of received packet
	size_t rbuflen_ ;		// length of data received (<= mtu_)
} ;

#endif
