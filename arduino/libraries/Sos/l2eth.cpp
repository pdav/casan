#include "l2eth.h"

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
#define	ETH_OFFSET_ETHTYPE	12
#define	ETH_OFFSET_SIZE		14		// specific to SOS
#define	ETH_OFFSET_PAYLOAD	16

#define	ETH_CRC_LENGTH		4

#define	ETH_MTU			1500


/******************************************************************************
 * l2addr_eth methods
 */

// constructor
l2addr_eth::l2addr_eth ()
{
}

// constructor
l2addr_eth::l2addr_eth (const char *a)
{
    int i = 0 ;
    byte b = 0 ;

    while (*a != '\0' && i < ETHADDRLEN)
    {
	if (*a == ':')
	{
	    addr_ [i++] = b ;
	    b = 0 ;
	}
	else if (isxdigit (*a))
	{
	    byte x ;
	    char c ;

	    c = tolower (*a) ;
	    x = isdigit (c) ? (c - '0') : (c - 'a' + 10) ;
	    b = (b << 4) + x ;
	}
	else
	{
	    for (i = 0 ; i < ETHADDRLEN ; i++)
		addr_ [i] = 0 ;
	    break ;
	}
	a++ ;
    }
    if (i < ETHADDRLEN)
	addr_ [i] = b ;
}

// copy constructor
l2addr_eth::l2addr_eth (const l2addr_eth &x)
{
    memcpy (addr_, x.addr_, ETHADDRLEN) ;
}

// assignment operator
l2addr_eth & l2addr_eth::operator= (const l2addr_eth &x)
{
    if (addr_ == x.addr_)
	return *this ;
    memcpy (addr_, x.addr_, ETHADDRLEN) ;
    return *this ;
}

bool l2addr_eth::operator== (const l2addr &other)
{
    l2addr_eth *oe = (l2addr_eth *) &other ;
    return memcmp (this->addr_, oe->addr_, ETHADDRLEN) == 0 ;
}

bool l2addr_eth::operator!= (const l2addr &other)
{
    l2addr_eth *oe = (l2addr_eth *) &other ;
    return memcmp (this->addr_, oe->addr_, ETHADDRLEN) != 0 ;
}

// Raw MAC address (array of 6 bytes)
bool l2addr_eth::operator!= (const unsigned char *other)
{
    return memcmp (this->addr_, other, ETHADDRLEN) != 0 ;
}

void l2addr_eth::set_raw_addr (const unsigned char *mac_addr) 
{
    memcpy (this->addr_, mac_addr, ETHADDRLEN) ;
}

unsigned char * l2addr_eth::get_raw_addr (void) 
{
    return this->addr_ ;
}

void l2addr_eth::print (void) {
    int i ;

    Serial.print (F ("Eth : \033[32m")) ;
    for (i = 0 ; i < ETHADDRLEN ; i++)
    {
	if (i > 0)
	    Serial.print (':') ;
	Serial.print (addr_ [i], HEX) ;
    }
    Serial.print (F ("\033[00m")) ;
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

void l2net_eth::start (l2addr *a, bool promisc, size_t mtu, int ethtype)
{
    int cmd ;

    myaddr_ = * (l2addr_eth *) a ;
    ethtype_ = ethtype ;
    if (mtu == 0)
	mtu = ETH_MTU - 2 ;		// exluding MAC header + SOS length
    mtu_ = mtu ;

    if (rbuf_ != NULL)
	free (rbuf_) ;
    rbuf_ = (byte *) malloc (mtu_ + ETH_OFFSET_PAYLOAD) ;

    W5100.init () ;
    W5100.setMACAddress (myaddr_.get_raw_addr ()) ;
    cmd = SnMR::MACRAW ;
    if (! promisc)
	cmd |= S0_MR_MF ;		// MAC filter for socket 0
    W5100.writeSnMR (SOCK0, cmd) ;
    W5100.execCmdSn (SOCK0, Sock_OPEN) ;
}

/*
 * Send packet
 *
 * Returns true if data is sent (i.e. located in the send buffer of
 * the W5100 chip). If message should be truncated, it is not sent.
 */

bool l2net_eth::send (l2addr &dest, const uint8_t *data, size_t len) 
{
    bool success = false ;

    if (len + ETH_OFFSET_PAYLOAD <= mtu_)
    {
	l2addr_eth *m = (l2addr_eth *) &dest ;
	byte *sbuf ;		// send buffer
	size_t sbuflen ;	// buffer size
	size_t paylen ;		// restricted to mtu, if any

	paylen = len ;		// keep original length
	sbuflen = len + ETH_OFFSET_PAYLOAD ;	// add MAC header + SOS length
	sbuf = (byte *) malloc (sbuflen) ;

	// Standard Ethernet MAC header (14 bytes)
	memcpy (sbuf + ETH_OFFSET_DST_ADDR, m->get_raw_addr (), ETHADDRLEN) ;
	memcpy (sbuf + ETH_OFFSET_SRC_ADDR, myaddr_.get_raw_addr (), ETHADDRLEN) ;
	sbuf [ETH_OFFSET_ETHTYPE   ] = (char) ((ethtype_ >> 8) & 0xff) ;
	sbuf [ETH_OFFSET_ETHTYPE +1] = (char) ((ethtype_     ) & 0xff) ;

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
	byte hdr [ETH_OFFSET_PAYLOAD] ;

	// Standard Ethernet MAC header (14 bytes)
	memcpy (hdr + ETH_OFFSET_DST_ADDR, m->get_raw_addr (), ETHADDRLEN) ;
	memcpy (hdr + ETH_OFFSET_SRC_ADDR, myaddr_.get_raw_addr (), ETHADDRLEN) ;
	hdr [ETH_OFFSET_ETHTYPE   ] = (char) ((ethtype_ >> 8) & 0xff) ;
	hdr [ETH_OFFSET_ETHTYPE +1] = (char) ((ethtype_     ) & 0xff) ;

	// SOS message size (2 bytes)
	hdr [ETH_OFFSET_SIZE    ] = BYTE_HIGH (len + 2) ;
	hdr [ETH_OFFSET_SIZE + 1] = BYTE_LOW  (len + 2) ;

	// Place header in buffer
	W5100.send_data_processing (SOCK0, hdr, ETH_OFFSET_PAYLOAD) ;

	// Place payload in buffer
	W5100.send_data_processing (SOCK0, data, len) ;

	// Send packet
	W5100.execCmdSn (SOCK0, Sock_SEND_MAC) ;
#endif

	success = true ;
    }

    return success ;
}

/*
 * Receive packet
 *
 * returns L2_RECV_EMPTY if no packet has been received
 * returns L2_RECV_WRONG_DEST if destination address is wrong
 *      (i.e. not our MAC address nor the broadcast address)
 * returns L2_RECV_WRONG_ETHTYPE if Ethernet packet type is not the good one
 * returns L2_RECV_TRUNCATED if packet is too large (i.e. has been truncated)
 * returns L2_RECV_RECV_OK if ok
 */

l2_recv_t l2net_eth::recv (void) 
{
    l2_recv_t r ;
    size_t wizlen ;

    wizlen = W5100.getRXReceivedSize (SOCK0) ;	// size of data in RX mem
    if (wizlen == 0)
	r = L2_RECV_EMPTY ;
    else
    {
	uint8_t pl [2] ;		// intermediate storage for pkt len
	size_t remaining ;

	r = L2_RECV_RECV_OK ;		// default value

	// Extract packet length
	W5100.recv_data_processing (SOCK0, pl, sizeof pl, 0) ;
	W5100.execCmdSn (SOCK0, Sock_RECV) ;
	pktlen_ = INT16 (pl [0], pl [1]) ;
	pktlen_ -= 2 ;			// w5100 header includes size itself

	if (pktlen_ <= mtu_ + ETH_OFFSET_PAYLOAD) 
	{
	    rbuflen_ = pktlen_ ;
	    remaining = 0 ;
	}
	else
	{
	    rbuflen_ = mtu_ + ETH_OFFSET_PAYLOAD ;
	    remaining = pktlen_ - rbuflen_ ;
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

	// Check destination address
	if (myaddr_ != rbuf_ + ETH_OFFSET_DST_ADDR
		&& l2addr_eth_broadcast != rbuf_ + ETH_OFFSET_DST_ADDR)
	    r = L2_RECV_WRONG_DEST ;

	// Check Ethernet type
	if (r == L2_RECV_RECV_OK
		&& (rbuf_ [ETH_OFFSET_ETHTYPE] != BYTE_HIGH (ethtype_)
		 || rbuf_ [ETH_OFFSET_ETHTYPE + 1] != BYTE_LOW (ethtype_) ))
	    r = L2_RECV_WRONG_ETHTYPE ;

	// Truncated?
	if (r == L2_RECV_RECV_OK && pktlen_ > mtu_)
	    r = L2_RECV_TRUNCATED ;
    }

    return r ;
}

l2addr *l2net_eth::bcastaddr (void)
{
    return &l2addr_eth_broadcast ;
}

void l2net_eth::get_src (l2addr *mac) 
{
    l2addr_eth *a = (l2addr_eth *) mac ;
    a->set_raw_addr (rbuf_ + ETH_OFFSET_SRC_ADDR) ;
}

void l2net_eth::get_dst (l2addr *mac) 
{
    l2addr_eth *a = (l2addr_eth *) mac ;
    a->set_raw_addr (rbuf_ + ETH_OFFSET_DST_ADDR) ;
}

l2addr *l2net_eth::get_src (void)
{
    l2addr_eth *a = new l2addr_eth ;
    get_src (a) ;
    return a ;
}

l2addr *l2net_eth::get_dst (void)
{
    l2addr_eth *a = new l2addr_eth ;
    get_dst (a) ;
    return a ;
}

uint8_t *l2net_eth::get_payload (int offset) 
{
    return rbuf_ + ETH_OFFSET_PAYLOAD + offset ;
}

size_t l2net_eth::get_paylen (void) 
{
    size_t sos_paylen ;

    sos_paylen = INT16 (rbuf_ [ETH_OFFSET_SIZE], rbuf_ [ETH_OFFSET_SIZE + 1]) ;
    return sos_paylen - 2 ;		// SOS size includes size itself
}

void l2net_eth::dump_packet (size_t start, size_t maxlen)
{
    size_t i, n ;

    n = start + maxlen ;
    if (rbuflen_ < n)
	n = rbuflen_ ;

    for (i = start ; i < n ; i++)
    {
	if (i > start)
	    Serial.print (' ') ;
	Serial.print ((rbuf_ [i] >> 4) & 0xf, HEX) ;
	Serial.print ((rbuf_ [i]     ) & 0xf, HEX) ;
    }

    Serial.println () ;
}
