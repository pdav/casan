/**
 * @file l2-eth.h
 * @brief l2addr and l2net specializations for Ethernet
 */

#ifndef L2_ETH_H
#define	L2_ETH_H

/*
 * These classes are used for Ethernet specialization
 */

#include "Arduino.h"
#include "l2.h"
#include "defs.h"
#include "w5100.h"

/** Ethernet frame type used for SOS frames */
#define	ETHTYPE		0x88b5

/** Ethernet address length */
#define	ETHADDRLEN	6

/** Ethernet MTU (complete MAC frame length, including MAC header and footer) */
#define	ETHMTU		1518

/**
 * @brief Specialization of the l2addr abstract class for Ethernet
 *
 * This class provides real methods (specified in l2addr virtual
 * class) for Ethernet addresses.
 */

class l2addr_eth : public l2addr
{
    public:
	l2addr_eth () ;					// constructor
	l2addr_eth (const char *) ;			// constructor
	l2addr_eth (const l2addr_eth &) ;		// copy constructor
	l2addr_eth &operator= (const l2addr_eth &) ;	// copy assignment

	bool operator== (const l2addr &) ;
	bool operator!= (const l2addr &) ;
	bool operator!= (const unsigned char *) ;	// 6 bytes
	void print (void) ;

    protected:
	byte addr_ [ETHADDRLEN] ;

	friend class l2net_eth ;
} ;

extern l2addr_eth l2addr_eth_broadcast ;

/**
 * @brief Specialization of the l2net abstract class for Ethernet
 *
 * This class provides real methods (specified in l2net virtual
 * class) for Ethernet networks. It handles the lowest layer,
 * which is assumed to be the Wiznet W5100 Ethernet chip found
 * on the Arduino Ethernet shield. As such, it needs the SPI and
 * Ethernet standard Arduino libraries.
 */

class l2net_eth : public l2net
{
    public:
	l2net_eth (void) ;
	~l2net_eth () ;
	void start (l2addr *myaddr, bool promisc, size_t mtu, int ethtype) ;
	bool send (l2addr &dest, const uint8_t *data, size_t len) ;
	// the "recv" method copy the received packet in
	// the instance private variable (see rbuf_/rbuflen_ below)
	l2_recv_t recv (void) ;

	l2addr *bcastaddr (void) ;	// return a static variable
	l2addr *get_src (void) ;	// get a new l2addr_eth
	l2addr *get_dst (void) ;	// get a new l2addr_eth

	// Ethernet payload (not including MAC header, of course)
	uint8_t *get_payload (int offset) ;
	size_t get_paylen (void) ;	// if truncated pkt: truncated payload

	// debug usage
	void dump_packet (size_t start, size_t maxlen) ;

    private:
	l2addr_eth myaddr_ ;		// my MAC address
	int ethtype_ ;
	byte *rbuf_ ;			// receive buffer (of size mtu_ bytes)
	size_t pktlen_ ;		// real length of received packet
	size_t rbuflen_ ;		// length of data received (<= mtu_)
} ;

#endif
