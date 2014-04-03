#ifndef L2ETH_H
#define	L2ETH_H

/*
 * These classes are used for Ethernet specialization
 */

#include "Arduino.h"
#include "l2.h"
#include "defs.h"
#include "utility/w5100.h"
#include "enum.h"
#include "memory_free.h"

// Ethernet address length
#define	ETHADDRLEN	6

// Various fields in Ethernet frame
#define	OFFSET_DEST_ADDR	(0)
#define	OFFSET_SRC_ADDR		(6)
#define	OFFSET_ETHTYPE		(12)
#define	OFFSET_SIZE		(14)
#define	OFFSET_DATA		(16)
#define	OFFSET_RECEPTION	(2)

class l2addr_eth : public l2addr
{
    public:
	l2addr_eth () ;					// constructor
	l2addr_eth (const char *) ;			// constructor
	l2addr_eth (const l2addr_eth &) ;		// copy constructor
	l2addr_eth &operator= (const l2addr_eth &) ;	// copy assignment

	bool operator== (const l2addr &) ;
	bool operator!= (const l2addr &) ;
	bool operator!= (const unsigned char* mac_addr) ;
	void print () ;

	void set_addr (const unsigned char* mac_addr) ;
	unsigned char *get_raw_addr (void) ;

    private:
	byte addr_ [ETHADDRLEN] ;
} ;

extern l2addr_eth l2addr_eth_broadcast ;

/*
 * this class handles the lowest layer, it will talk directly with the 
 * arduino ethernet shield
 */
class l2net_eth : public l2net
{
    public:
	l2net_eth (l2addr *mac_addr, size_t mtu, int ethtype) ;
	~l2net_eth () ;
	size_t send (l2addr &mac_dest, const uint8_t *data, size_t len) ;
	// the "recv" method copy the received packet in
	// the instance private variable (see rbuf_/rbuflen_ below)
	l2_recv_t recv (void) ;

	l2addr *bcastaddr (void) ;
	void get_mac_src (l2addr * mac_src) ;
	uint8_t * get_offset_payload (int offset) ;
	size_t get_payload_length (void) ;

	void set_master_addr (l2addr * master_addr) ;

    private:
	SOCKET sock_ ;		 	// socket opened in RAW mode
	l2addr_eth *mac_addr_ ;
	l2addr_eth *master_addr_ ;	// master address for this slave
	int ethtype_ ;
	byte *rbuf_ ;			// receive buffer (of size mtu_ bytes)
	size_t rbuflen_ ;		// length of data received

	int available () ;
	void print_eth_addr (byte addr []) ;
	void print_packet (byte pkt [], int len) ;
	l2_recv_t check_received (void) ;
} ;

#endif
