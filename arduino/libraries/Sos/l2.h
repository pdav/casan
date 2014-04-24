/**
 * @file l2.h
 * @brief l2addr and l2net virtual class interfaces
 */

#ifndef	L2_H
#define	L2_H

/*
 * These classes are designed to virtualize specific L2 networks
 */

#include "defs.h"

/**
 * @class l2addr
 *
 * @brief Abstracts an address of any L2 network
 *
 * This class is a pure abstract class. It cannot be instancied.
 * Instead, the SOS engine keeps track of pointers to this class,
 * which are in reality pointers to objects of the `l2addr-xxx`
 * derived classes.
 */

class l2addr
{
    public:
	virtual ~l2addr () {} ;
	virtual bool operator== (const l2addr &other) = 0 ;
	virtual bool operator!= (const l2addr &other) = 0 ;
	virtual void print (void) = 0 ;
} ;

/**
 * @class l2net
 *
 * @brief Abstracts access to any L2 network
 *
 * This class is a pure abstract class. It cannot be instancied.
 * Instead, the SOS engine keeps track of the current network with
 * a pointer to this class, which is in reality a pointer to an
 * object of one of the `l2addr-xxx` derived classes, and it
 * performs all network accesses through this pointer.
 *
 * Each derived class provides some methods specific to the
 * L2 technology (specify Ethernet type for Ethernet, or channel
 * id for IEEE 802.15.4 for example), and a specific `start` method.
 */

class l2net {
    public:
	/** Return type for the `recv` methods in various classes
	 *
	 * This type is returned by the `recv` methods of the derived
	 * classes (`l2net-xxx`). They are not specialized for a
	 * particular L2 technology.
	 */

	typedef enum
	{
	    RECV_EMPTY, 		///< No received message
	    RECV_WRONG_TYPE,		///< Wrong Ethernet type, for example
	    RECV_WRONG_DEST,		///< Wrong destination address
	    RECV_TRUNCATED,		///< Truncated message
	    RECV_OK			///< Message received successfully
	} l2_recv_t ;

	virtual ~l2net () {} ;

	/** Send a L2 frame to the given destination address */
	virtual bool send (l2addr &dest, const uint8_t *data, size_t len) = 0 ;

	/** Receive a L2 frame
	 * @return return code (see l2net::l2_recv_t type)
	 */
	virtual l2_recv_t recv (void) = 0 ;

	/** Returns the broadcast address for this network
	 * @return a pointer to a l2addr address (do not free it)
	 */
	virtual l2addr *bcastaddr (void) = 0 ;		// global variable

	/** Get the source address from the received message
	 * @return a pointer to a new l2addr_xxx address (to be freed
	 * after use)
	 */
	virtual l2addr *get_src (void) = 0 ;		// get a new l2addr

	/** Get the destination address from the received message
	 * @return a pointer to a new l2addr_xxx address (to be freed
	 * after use)
	 */
	virtual l2addr *get_dst (void) = 0 ;		// get a new l2addr

	/** Get a pointer to the payload in the received message
	 * @param offset offset in payload
	 * @return pointer in the received message (do not free it)
	 */
	virtual uint8_t *get_payload (int offset) = 0 ;	// ptr in message

	/** Get the payload length of the received message */
	virtual size_t get_paylen (void) = 0 ;

	/** Get the MTU (maximum payload length, excluding headers) */
	size_t mtu (void) { return mtu_ ; }

    protected:
	// Note on MTU: this value does not include MAC header
	// thus, it is the maximum size of a SOS-level datagram
	size_t mtu_ ;		// must be initialized in derived classes
} ;

#endif
