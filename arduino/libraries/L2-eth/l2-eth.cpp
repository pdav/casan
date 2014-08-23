/**
 * @file l2-eth.cpp
 * @brief L2addr_eth and l2net_eth class implementations
 */

#include "l2-eth.h"

/*
 * Ethernet classes for SOS
 *
 * Needs : w5100 functions from standard (i.e. Arduino) Ethernet class
 */

l2addr_eth l2addr_eth_broadcast ("ff:ff:ff:ff:ff:ff") ;

#define	SOCK0		((SOCKET) 0)		// mandatory for MAC raw
#define	S0_MR_MF	(1<<6)			// MAC filter for socket 0

#define	FLUSH_SIZE	128

// Various fields in Ethernet frame
#define	ETH_OFFSET_DST_ADDR	0
#define	ETH_OFFSET_SRC_ADDR	6
#define	ETH_OFFSET_TYPE		12
#define	ETH_OFFSET_SIZE		14		// specific to SOS
#define	ETH_OFFSET_PAYLOAD	16

#define	ETH_SIZE_HEADER		ETH_OFFSET_PAYLOAD
#define	ETH_SIZE_FCS		4

/******************************************************************************
 * l2addr_eth methods
 */

// constructor
l2addr_eth::l2addr_eth ()
{
}

/** Constructor with an address given as a string
 *
 * This constructor is used to initialize an address with a
 * string such as "`01:02:03:04:05:06`".
 *
 * @param a address to be parsed
 */

l2addr_eth::l2addr_eth (const char *a)
{
    int i = 0 ;
    uint8_t b = 0 ;

    while (*a != '\0' && i < ETH_ADDRLEN)
    {
	if (*a == ':')
	{
	    addr_ [i++] = b ;
	    b = 0 ;
	}
	else if (isxdigit (*a))
	{
	    uint8_t x ;
	    char c ;

	    c = tolower (*a) ;
	    x = isdigit (c) ? (c - '0') : (c - 'a' + 10) ;
	    b = (b << 4) + x ;
	}
	else
	{
	    for (i = 0 ; i < ETH_ADDRLEN ; i++)
		addr_ [i] = 0 ;
	    break ;
	}
	a++ ;
    }
    if (i < ETH_ADDRLEN)
	addr_ [i] = b ;
}

// copy constructor
l2addr_eth::l2addr_eth (const l2addr_eth &x)
{
    memcpy (addr_, x.addr_, ETH_ADDRLEN) ;
}

// assignment operator
l2addr_eth & l2addr_eth::operator= (const l2addr_eth &x)
{
    if (addr_ == x.addr_)
	return *this ;
    memcpy (addr_, x.addr_, ETH_ADDRLEN) ;
    return *this ;
}

bool l2addr_eth::operator== (const l2addr &other)
{
    l2addr_eth *oe = (l2addr_eth *) &other ;
    return memcmp (this->addr_, oe->addr_, ETH_ADDRLEN) == 0 ;
}

bool l2addr_eth::operator!= (const l2addr &other)
{
    l2addr_eth *oe = (l2addr_eth *) &other ;
    return memcmp (this->addr_, oe->addr_, ETH_ADDRLEN) != 0 ;
}

// Raw MAC address (array of 6 bytes)
bool l2addr_eth::operator!= (const unsigned char *other)
{
    return memcmp (this->addr_, other, ETH_ADDRLEN) != 0 ;
}

void l2addr_eth::print (void)
{
    int i ;

    for (i = 0 ; i < ETH_ADDRLEN ; i++)
    {
	if (i > 0)
	    DBG1 (':') ;
	DBG2 (addr_ [i], HEX) ;
    }
}

/******************************************************************************
 * l2net_eth methods
 */

l2net_eth::l2net_eth (void)
{
}

l2net_eth::~l2net_eth () 
{
    if (rbuf_ != NULL)
	free (rbuf_) ;
}

/**
 * @brief Start an Ethernet network socket
 *
 * Initialize the Ethernet network access with needed constants.
 *
 * @param a Our Ethernet address
 * @param promisc True if we want to access this network in promisc mode
 * @param ethtype Ethernet type used for sending and receiving frames
 */

void l2net_eth::start (l2addr *a, bool promisc, int ethtype)
{
    int cmd ;

    myaddr_ = * (l2addr_eth *) a ;
    ethtype_ = ethtype ;
    mtu_ = ETH_MTU ;

    if (rbuf_ != NULL)
    {
	free (rbuf_) ;
	rbuf_ = NULL ;
    }
    rbufsize_ = 0 ;

    W5100.init () ;
    W5100.setMACAddress (myaddr_.addr_) ;
    cmd = SnMR::MACRAW ;
    if (! promisc)
	cmd |= S0_MR_MF ;		// MAC filter for socket 0
    W5100.writeSnMR (SOCK0, cmd) ;
    W5100.execCmdSn (SOCK0, Sock_OPEN) ;
}

/**
 * @brief Get MAC payload length
 *
 * This method returns the current MAC payload length, i.e. MTU without
 * MAC header and trailer.
 */

size_t l2net_eth::maxpayload (void)
{
    return mtu_ - (ETH_SIZE_HEADER + ETH_SIZE_FCS) ; // excl. MAC hdr + SOS len
}

/**
 * @brief Send a packet on the Ethernet network
 *
 * This method encapsulates the SOS packet in an Ethernet frame, copy
 * it in the W5100 shared buffer and order the W5100 to send the frame.
 * Since the frame will be sent later, a return value of `true` just
 * indicates that the frame has correctly been sent to the chip, not
 * that the frame has been successfully transmitted on the wire.
 * 
 * This methods adds a two byte length in front of the SOS payload
 * (just after the Ethernet header). This is done specifically for
 * the Ethernet since SOS packets may be shorter than minimal size
 * and some drivers cannot report the "true" frame length.
 *
 * See the l2net::send method for parameters and return value.
 *
 * @bug this method assumes that there is enough room in the shared
 *	buffer, which may not be true
 * @bug memory is temporarily allocated to build the frame, which
 *	makes a pressure on the controller memory. This could perhaps
 *	be avoided if the `W5100::send_data_processing` method did not
 *	send an order.
 */

bool l2net_eth::send (l2addr &dest, const uint8_t *data, size_t len) 
{
    bool success = false ;

    if (len + ETH_SIZE_HEADER <= mtu_ - ETH_SIZE_FCS)
    {
	l2addr_eth *m = (l2addr_eth *) &dest ;
	uint8_t *sbuf ;		// send buffer
	size_t sbuflen ;	// buffer size
	size_t paylen ;		// restricted to mtu, if any

	paylen = len ;		// keep original length
	sbuflen = len + ETH_SIZE_HEADER ;	// add MAC header + SOS length
	sbuf = (uint8_t *) malloc (sbuflen) ;

	// Standard Ethernet MAC header (14 bytes)
	memcpy (sbuf + ETH_OFFSET_DST_ADDR, m->addr_, ETH_ADDRLEN) ;
	memcpy (sbuf + ETH_OFFSET_SRC_ADDR, myaddr_.addr_, ETH_ADDRLEN) ;
	sbuf [ETH_OFFSET_TYPE   ] = (char) ((ethtype_ >> 8) & 0xff) ;
	sbuf [ETH_OFFSET_TYPE +1] = (char) ((ethtype_     ) & 0xff) ;

	// SOS message size (2 bytes)
	sbuf [ETH_OFFSET_SIZE    ] = BYTE_HIGH (paylen + 2) ;
	sbuf [ETH_OFFSET_SIZE + 1] = BYTE_LOW  (paylen + 2) ;

	// Payload
	memcpy (sbuf + ETH_OFFSET_PAYLOAD, data, paylen) ;

	// Send packet
	W5100.send_data_processing (SOCK0, sbuf, sbuflen) ;
	W5100.execCmdSn (SOCK0, Sock_SEND_MAC) ;

	free (sbuf) ;
#if 0
	/*
	 * This code differs from the previous one by attempting
	 * to avoid the use of new buffer to store the whole
	 * message. Instead, it tries to send 2 parts (header +
	 * payload).
	 * It does not work. No time to investigate, and the
	 * Wiz5100 datasheet is not helpful.
	 */

	l2addr_eth *m = (l2addr_eth *) &dest ;
	uint8_t hdr [ETH_SIZE_HEADER] ;

	// Standard Ethernet MAC header (14 bytes)
	memcpy (hdr + ETH_OFFSET_DST_ADDR, m->addr_, ETH_ADDRLEN) ;
	memcpy (hdr + ETH_OFFSET_SRC_ADDR, myaddr_.addr_, ETH_ADDRLEN) ;
	hdr [ETH_OFFSET_TYPE   ] = (char) ((ethtype_ >> 8) & 0xff) ;
	hdr [ETH_OFFSET_TYPE +1] = (char) ((ethtype_     ) & 0xff) ;

	// SOS message size (2 bytes)
	hdr [ETH_OFFSET_SIZE    ] = BYTE_HIGH (len + 2) ;
	hdr [ETH_OFFSET_SIZE + 1] = BYTE_LOW  (len + 2) ;

	// Place header in buffer
	W5100.send_data_processing (SOCK0, hdr, ETH_SIZE_HEADER) ;

	// Place payload in buffer
	W5100.send_data_processing (SOCK0, data, len) ;

	// Send packet
	W5100.execCmdSn (SOCK0, Sock_SEND_MAC) ;
#endif

	success = true ;
    }

    return success ;
}

/**
 * @brief Receive a packet from the Ethernet network
 *
 * This method queries the W5100 chip in order to get a received
 * Ethernet frame. The frame is copied in a buffer dynamically
 * allocated.
 *
 * See the `l2net::l2_recv_t` enumeration for return values.
 */

l2net::l2_recv_t l2net_eth::recv (void) 
{
    l2_recv_t r ;
    size_t wizlen ;

    if (rbufsize_ != mtu_ - ETH_SIZE_FCS) // MTU change, or initial allocation
    {
	if (rbuf_ != NULL)
	    free (rbuf_) ;
	rbufsize_ = mtu_ - ETH_SIZE_FCS ;
	rbuf_ = (uint8_t *) malloc (rbufsize_) ;
    }

    wizlen = W5100.getRXReceivedSize (SOCK0) ;	// size of data in RX mem
    if (wizlen == 0)
	r = RECV_EMPTY ;
    else
    {
	uint8_t pl [2] ;		// intermediate storage for pkt len
	size_t remaining ;

	r = RECV_OK ;		// default value

	// Extract w5100 header == length (padded to 60 for a small packet)
	W5100.recv_data_processing (SOCK0, pl, sizeof pl, 0) ;
	W5100.execCmdSn (SOCK0, Sock_RECV) ;
	pktlen_ = INT16 (pl [0], pl [1]) ;
	pktlen_ -= 2 ;			// w5100 header includes size itself

	if (pktlen_ <= rbufsize_) 
	{
	    rbuflen_ = pktlen_ ;
	    remaining = 0 ;
	}
	else
	{
	    rbuflen_ = rbufsize_ ;
	    remaining = pktlen_ - rbuflen_ ;
	    r = RECV_TRUNCATED ;
	}

	// copy received packet from W5100 internal buffer
	// to private instance variable
	W5100.recv_data_processing (SOCK0, rbuf_, rbuflen_, 0) ;
	W5100.execCmdSn (SOCK0, Sock_RECV) ;

	// don't try to consume more bytes than effectively received
	wizlen -= rbuflen_ + 2 ;
	if (remaining > wizlen)
	    remaining = wizlen ;

	// flush remaining bytes if packet was too big
	while (remaining > 0)
	{
	    uint8_t flushbuf [FLUSH_SIZE] ;
	    size_t n ;

	    n = (remaining > sizeof flushbuf) ? sizeof flushbuf : remaining ;
	    W5100.recv_data_processing (SOCK0, flushbuf, n, 0) ;
	    W5100.execCmdSn (SOCK0, Sock_RECV) ;
	    remaining -= n ;
	}

	// Get true packet length (specific length for Ethernet SOS payload)
	pktlen_ = INT16 (rbuf_ [ETH_OFFSET_SIZE], rbuf_ [ETH_OFFSET_SIZE+1]) ;
	pktlen_ -= 2 ;

	// XXX HOW CAN THIS TEST (l2addr_eth*  != uint8_t *) WORK?
	// Check destination address
	if (myaddr_ != rbuf_ + ETH_OFFSET_DST_ADDR
		&& l2addr_eth_broadcast != rbuf_ + ETH_OFFSET_DST_ADDR)
	    r = RECV_WRONG_DEST ;

	// Check Ethernet type
	if (r == RECV_OK
		&& (rbuf_ [ETH_OFFSET_TYPE] != BYTE_HIGH (ethtype_)
		 || rbuf_ [ETH_OFFSET_TYPE + 1] != BYTE_LOW (ethtype_) ))
	    r = RECV_WRONG_TYPE ;
    }

    return r ;
}

/**
 * @brief Returns the broadcast Ethernet address
 *
 * The broadcast Ethernet address is located in a global variable.
 * This method returns its address.
 *
 * @return address of an existing l2addr_eth object (do not free it)
 */

l2addr *l2net_eth::bcastaddr (void)
{
    return &l2addr_eth_broadcast ;
}

/**
 * @brief Returns the source address of the received frame
 *
 * This methods creates a new l2addr_eth and initializes it with
 * the source address from the currently received frame.
 *
 * @return address of a new l2addr_eth object (to delete after use)
 */

l2addr *l2net_eth::get_src (void)
{
    l2addr_eth *a = new l2addr_eth ;
    memcpy (a->addr_, rbuf_ + ETH_OFFSET_SRC_ADDR, ETH_ADDRLEN) ;
    return a ;
}

/**
 * @brief Returns the destination address of the received frame
 *
 * This methods creates a new l2addr_eth and initializes it with
 * the destination address from the currently received frame.
 *
 * @return address of a new l2addr_eth object (to delete after use)
 */

l2addr *l2net_eth::get_dst (void)
{
    l2addr_eth *a = new l2addr_eth ;
    memcpy (a->addr_, rbuf_ + ETH_OFFSET_DST_ADDR, ETH_ADDRLEN) ;
    return a ;
}

/**
 * @brief Returns the address of the received payload
 *
 * This methods returns the address of the payload inside the
 * received frame (payload is not copied). Note that payload
 * returned does not include the payload added specifically for
 * Ethernet frames (see l2net_eth::send).
 *
 * @return address inside an existing buffer (do not free it)
 */

uint8_t *l2net_eth::get_payload (int offset) 
{
    return rbuf_ + ETH_OFFSET_PAYLOAD + offset ;
}

/**
 * @brief Returns the payload length
 *
 * This methods returns the original length of the received
 * frame. Even if the frame has been truncated on reception,
 * the payload returned is the true payload.
 *
 * @return original length
 */

size_t l2net_eth::get_paylen (void)		// original length
{
    return pktlen_ ;
}

/**
 * @brief Dump some parts of a frame
 *
 * This methods prints a part of the received frame.
 */

void l2net_eth::dump_packet (size_t start, size_t maxlen)
{
    size_t i, n ;

    n = start + maxlen ;
    if (rbuflen_ < n)
	n = rbuflen_ ;

    for (i = start ; i < n ; i++)
    {
	if (i > start)
	    DBG1 (' ') ;
	DBG2 ((rbuf_ [i] >> 4) & 0xf, HEX) ;
	DBG2 ((rbuf_ [i]     ) & 0xf, HEX) ;
    }

    DBGLN0 () ;
}
