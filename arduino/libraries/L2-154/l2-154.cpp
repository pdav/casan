/**
 * @file l2-154.cpp
 * @brief L2addr_154 and l2net_154 class implementations
 */

#include "l2-154.h"

/*
 * 802.15.4 classes for SOS
 *
 * Needs : ZigMsg functions from associated library
 */

l2addr_154 l2addr_154_broadcast ("ff:ff") ;

ZigMsg::addr2_t addr2_broadcast = CONST16 (0xff, 0xff) ;

/*
 * We assume a fixed size MAC header which includes:
 * 2 bytes : FCF (with the intra-PAN bit set)
 * 1 byte  : Sequence number
 * 2 bytes : destination address
 * 2 bytes : destination PANID
 * 2 bytes : source address (without PANID since the intra-PAN bit is set)
 * and no auxiliary security header field
 *
 * The MAC footer includes a 2 bytes checksum.
 *
 * Hence, the MSDU (MAC service Data Unit) length is 127 - header - fcs
 */

#define	I154_SIZE_HEADER	(2+1+2+2+2)
#define	I154_SIZE_FCS		2		// CRC-16 checksum at the end

/******************************************************************************
 * l2addr_154 methods
 */

// constructor
l2addr_154::l2addr_154 ()
{
}

/** Constructor with an address given as a string
 *
 * This constructor is used to initialize an address with a
 * string such as "`ca:fe`". 64-bits addresses are not supported.
 *
 * @param a address to be parsed
 */

l2addr_154::l2addr_154 (const char *a)
{
    int i = 0 ;
    uint8_t b = 0 ;
    uint8_t buf [I154_ADDRLEN] ;

    /*
     * General loop, when 8-byte addresses will be supported
     */

    while (*a != '\0' && i < I154_ADDRLEN)
    {
	if (*a == ':')
	{
	    buf [i++] = b ;
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
	    for (i = 0 ; i < I154_ADDRLEN ; i++)
		buf [i] = 0 ;
	    break ;
	}
	a++ ;
    }
    if (i < I154_ADDRLEN)
	buf [i] = b ;

    addr_ = CONST16 (buf [0], buf [1]) ;
}

// copy constructor
l2addr_154::l2addr_154 (const l2addr_154 &x)
{
    addr_ = x.addr_ ;
}

// assignment operator
l2addr_154 & l2addr_154::operator= (const l2addr_154 &x)
{
    addr_ = x.addr_ ;
    return *this ;
}

bool l2addr_154::operator== (const l2addr &other)
{
    l2addr_154 *oe = (l2addr_154 *) &other ;
    return this->addr_ == oe->addr_ ;
}

bool l2addr_154::operator!= (const l2addr &other)
{
    l2addr_154 *oe = (l2addr_154 *) &other ;
    return this->addr_ != oe->addr_ ;
}

void l2addr_154::print (void)
{
    Serial.print (BYTE_LOW (addr_), HEX) ;	// first byte: low bits
    Serial.print (':') ;
    Serial.print (BYTE_HIGH (addr_), HEX) ;	// second byte: high bits
}

/******************************************************************************
 * l2net_154 methods
 */

l2net_154::l2net_154 (void)
{
}

l2net_154::~l2net_154 () 
{
}

/**
 * @brief Start an IEEE 802.15.4 network socket
 *
 * Initialize the IEEE 802.15.4 network access with needed constants.
 *
 * @param a Our IEEE 802.15.4 address
 * @param promisc True if we want to access this network in promisc mode
 * @param chan channel number
 * @param panid PAN-id
 */

void l2net_154::start (l2addr *a, bool promisc, channel_t chan, panid_t panid)
{
    myaddr_ = ((l2addr_154 *) a)->addr_ ;
    zigmsg.addr2 (myaddr_) ;
    zigmsg.channel (chan) ;
    zigmsg.panid (panid) ;
    zigmsg.promiscuous (false) ;

    mtu_ = I154_MTU ;

    curframe_ = NULL ;			// no currently received frame

    zigmsg.start () ;
}

/*
 * @brief Get MAC payload length
 *
 * This method returns the current MAC payload length, i.e. MTU without
 * MAC header and trailer.
 */

size_t l2net_154::maxpayload (void)
{
    return mtu_ - (I154_SIZE_HEADER + I154_SIZE_FCS) ; // excl. MAC header
}

/**
 * @brief Send a packet on the IEEE 802.15.4 network
 *
 * This method sends the packet to the ZigMsg library in order to
 * send it.
 *
 * See the l2net::send method for parameters and return value.
 */

bool l2net_154::send (l2addr &dest, const uint8_t *data, size_t len) 
{
    bool success = false ;

    if (len <= mtu_ - (I154_SIZE_HEADER + I154_SIZE_FCS))
	success = zigmsg.sendto (((l2addr_154 *) &dest)->addr_, len, data) ;

    return success ;
}

/**
 * @brief Receive a packet from the IEEE 802.15.4 network
 *
 * This method queries the ZigMsg library in order. The received
 * packet is kept by the ZigMsg library in a private buffer.
 * Minimal decoding is done here in order to decide if it is
 * a valid packet (i.e. uses only 16-bits address and an Intra-PAN
 * bit).
 *
 * See the `l2net::l2_recv_t` enumeration for return values.
 */

l2net::l2_recv_t l2net_154::recv (void) 
{
    l2_recv_t r ;

    if (curframe_ != NULL)
	zigmsg.skip_received () ;

    curframe_ = zigmsg.get_received () ;
    if (curframe_ != NULL
	    && curframe_->frametype == Z_FT_DATA
	    && Z_GET_DST_ADDR_MODE (curframe_->fcf) == Z_ADDRMODE_ADDR2
	    && Z_GET_SRC_ADDR_MODE (curframe_->fcf) == Z_ADDRMODE_ADDR2
	    && Z_GET_INTRA_PAN (curframe_->fcf)
	    )
    {
	if (curframe_->dstaddr != myaddr_ && curframe_->dstaddr != addr2_broadcast)
	    r = RECV_WRONG_DEST ;
	else
	    r = RECV_OK ;
    }
    else r = RECV_EMPTY ;

    return r ;
}

/**
 * @brief Returns the broadcast IEEE 802.15.4 address
 *
 * The broadcast IEEE 802.15.4 address is located in a global variable.
 * This method returns its address.
 *
 * @return address of an existing l2addr_154 object (do not free it)
 */

l2addr *l2net_154::bcastaddr (void)
{
    return &l2addr_154_broadcast ;
}

/**
 * @brief Returns the source address of the received frame
 *
 * This methods creates a new l2addr_154 and initializes it with
 * the source address from the currently received frame.
 *
 * @return address of a new l2addr_154 object (to delete after use)
 */

l2addr *l2net_154::get_src (void)
{
    l2addr_154 *a = new l2addr_154 ;
    a->addr_ = curframe_->srcaddr ;
    return a ;
}

/**
 * @brief Returns the destination address of the received frame
 *
 * This methods creates a new l2addr_154 and initializes it with
 * the destination address from the currently received frame.
 *
 * @return address of a new l2addr_154 object (to delete after use)
 */

l2addr *l2net_154::get_dst (void)
{
    l2addr_154 *a = new l2addr_154 ;
    a->addr_ = curframe_->dstaddr ;
    return a ;
}

/**
 * @brief Returns the address of the received payload
 *
 * This methods returns the address of the payload inside the
 * received frame (payload is not copied).
 *
 * @return address inside an existing buffer (do not free it)
 */

uint8_t *l2net_154::get_payload (int offset) 
{
    return curframe_->payload ;
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

size_t l2net_154::get_paylen (void) 
{
    return curframe_->paylen ;
}

/**
 * @brief Dump some parts of a frame
 *
 * This methods prints a part of the received frame.
 */

void l2net_154::dump_packet (size_t start, size_t maxlen)
{
    size_t i, n ;

    n = start + maxlen ;
    if (curframe_->rawlen < n)
	n = curframe_->rawlen ;

    for (i = start ; i < n ; i++)
    {
	if (i > start)
	    Serial.print (' ') ;
	Serial.print ((curframe_->rawframe [i] >> 4) & 0xf, HEX) ;
	Serial.print ((curframe_->rawframe [i]     ) & 0xf, HEX) ;
    }

    Serial.println () ;
}
