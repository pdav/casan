#include <iostream>
#include <cstring>

// #define	_BSD_SOURCE			// for termios on Linux

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <termios.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "global.h"

#include "l2.h"
#include "l2802154.h"
#include "byte.h"

#define	XBEE_START		0x7e
#define	XBEE_TX_SHORT		0x01

#define	XBEE_MIN_FRAME_SIZE	5		// start:1+len:2+api:1+cksum:1
#define	XBEE_MAX_FRAME_SIZE	115		// RX 64bit addr, 100 bytes

#define	PRINT_HEX_DIGIT(os,c)	do { char d = (c) & 0xf ; d =  d < 10 ? d + '0' : d - 10 + 'a' ; (os) << d ; } while (false)

namespace sos {

/******************************************************************************
 * l2addr_802154 methods
 */

// default constructor
l2addr_802154::l2addr_802154 ()
{
}

// constructor
l2addr_802154::l2addr_802154 (const char *a)
{
    int i = 0 ;
    byte b = 0 ;

    while (*a != '\0' && i < L2802154ADDRLEN)
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
	    for (i = 0 ; i < L2802154ADDRLEN ; i++)
		addr_ [i] = 0 ;
	    break ;
	}
	a++ ;
    }
    if (i < L2802154ADDRLEN)
	addr_ [i++] = b ;

    /* Short addresses */
    if (i < L2802154ADDRLEN)
    {
	int j ;

	/* report the i bytes of address to the end of the byte string */
	for (j = 0 ; j < i ; j++)
	    addr_ [L2802154ADDRLEN - 1 - j] = addr_ [i - 1 - j] ;
	/* reset the L2802154ADDRLEN - i first bytes to 0 */
	for (j = 0 ; j < L2802154ADDRLEN - i ; j++)
	    addr_ [j] = 0 ;
    }
}

// copy constructor
l2addr_802154::l2addr_802154 (const l2addr_802154 &l)
{
    *this = l ;
}

// copy assignment constructor
l2addr_802154 &l2addr_802154::operator= (const l2addr_802154 &l)
{
    if (this != &l)
	*this = l ;
    return *this ;
}

l2addr_802154 l2addr_802154_broadcast ("ff:ff:ff:ff:ff:ff:ff:ff") ;

/******************************************************************************
 * Dump address
 */

void l2addr_802154::print (std::ostream &os) const
{
    for (int i = 0  ; i < L2802154ADDRLEN ; i++)
    {
	if (i > 0)
	    os << ":" ;
	PRINT_HEX_DIGIT (os, addr_ [i] >> 4) ;
	PRINT_HEX_DIGIT (os, addr_ [i]     ) ;
    }
}

/******************************************************************************
 * l2net_802154 methods
 */

bool l2addr_802154::operator== (const l2addr &other)
{
    l2addr_802154 *oe = (l2addr_802154 *) &other ;
    return std::memcmp (this->addr_, oe->addr_, L2802154ADDRLEN) == 0 ;
}

bool l2addr_802154::operator!= (const l2addr &other)
{
    return ! (*this == other) ;
}

int l2net_802154::init (const std::string iface, const char *type, const std::string myaddr, const std::string panid)
{
    std::string dev ;
    int n = -1 ;			// default: init fails
    l2addr_802154 a (myaddr.c_str ()) ;

    /* Various initializations */
    mtu_ = L2802154MTU ;
    maxlatency_ = L2802154MAXLATENCY ;

    /* Prepend /dev if needed */
    if (iface [0] == '/')
	dev = iface ;
    else
	dev = "/dev/" + iface ;

    if (strcmp (type, "xbee") == 0)
    {
	struct termios tm ;

	/* Open device */
	fd_ = open (dev.c_str (), O_RDWR) ;
	if (fd_ == -1)
	    return -1 ;

	/* Initialize RS-232 driver */
	if (tcgetattr (fd_, &tm) == -1)
	{
	    close (fd_) ;
	    return -1 ;
	}
	tm.c_iflag = IGNBRK | IGNPAR ;
	tm.c_oflag = 0 ;
	tm.c_cflag = CS8 | CREAD | B9600 ;
	tm.c_lflag = 0 ;
	for (int i = i ; i < NTAB (tm.c_cc) ; i++)
	    tm.c_cc [i] = _POSIX_VDISABLE ;
	if (tcsetattr (fd_, TCSANOW, &tm) == -1)
	{
	    close (fd_) ;
	    return -1 ;
	}

	/* Initialize device to API (non-escaped) mode */
	sleep (2) ;
	write (fd_, "+++", 3) ;
	sleep (2) ;
	write (fd_, "ATRE\r", 5) ;		// restore factory defaults
	// write (fd_, "ATFR\r", 5) ;		// software reset
	// initialize short addresses, my address, panid
	{
	    char buf [MAXBUF] ;
	    std::sprintf (buf, "ATMY%02x%02x\r",
			a.addr_ [L2802154ADDRLEN-2],
			a.addr_ [L2802154ADDRLEN-1]) ;
	    write (fd_, buf, strlen (buf)) ;
	}
	write (fd_, "ATAP1\r", 6) ;		// enter API mode
	write (fd_, "ATCN\r", 5) ;		// quit AT command mode
	
	n = 0 ;

	pbuffer_ = buffer_ ;
    }

    return n ;
}

void l2net_802154::term (void)
{
    close (fd_) ;
}

/******************************************************************************
 * Send data
 */

int l2net_802154::send (l2addr *daddr, void *data, int len)
{
    l2addr_802154 *da = (l2addr_802154 *) daddr ;
    int n ;
    byte cmd [MAXBUF] ;

    n = -1 ;
    if (len <= L2802154MTU)
    {
	int cmdlen ;

	cmdlen = sizeof cmd ;
	if (encode_transmit (cmd, cmdlen, da, (byte *) data, len))
	{
	    byte *pcmd ;

	    /*
	     * Send encoded command, may be in multiple chunks
	     * since write() may return a value < cmdlen
	     */

	    n = 0 ;
	    pcmd = cmd ;
	    while (cmdlen > 0)
	    {
		int r ;

		r = write (fd_, pcmd, cmdlen) ;
		if (r == -1)
		{
		    n = -1 ;
		    break ;
		}
		else
		{
		    cmdlen -= r ;
		    pcmd += r ;
		    n += r ;
		}
	    }
	}
    }
    return n ;
}

int l2net_802154::bsend (void *data, int len)
{
    return send (&l2addr_802154_broadcast, data, len) ;
}

// private method
bool l2net_802154::encode_transmit (byte *cmd, int &cmdlen, l2addr_802154 *daddr, byte *data, int len)
{
    byte *b ;
    int fdlen ;

    b = cmd ;
    // len of frame data (TX request) = tx:1+id:1+daddr:2+opt:1+data:len
    fdlen = 5 + len ;
    // len of encoded packet = start:1+len:2+frame:plen+cksum:1
    cmdlen = 4 + fdlen ;

    *b++ = XBEE_START ;
    *b++ = BYTE_HIGH (fdlen) ;
    *b++ = BYTE_LOW (fdlen) ;

    *b++ = XBEE_TX_SHORT ;
    // *b++ = 0 ;				// frame id
    *b++ = 0x41 ;				// frame id
    *b++ = daddr->addr_ [L2802154ADDRLEN-2] ;
    *b++ = daddr->addr_ [L2802154ADDRLEN-1] ;
    *b++ = 0 ;				// options
    std::memcpy (b, data, len) ;
    b += len ;

    *b++ = compute_checksum (cmd) ;

#if 0
    std::cout << "PKT=" ;
    for (int i = 0 ; i < cmdlen ; i++)
    {
	if (i > 0)
	    std::cout << ":" ;
	std::cout << std::hex << (int) cmd [i] ;
    }
    std::cout << "\n" << std::dec ;
#endif

    return true ;
}

// buf = encoded frame
int l2net_802154::compute_checksum (const byte *buf)
{
    int c = 0 ;
    int paylen ;

    // length of whole API packet, from API start byte to checksum included
    paylen = INT16 (buf [1], buf [2]) + 4 ;

    for (int i = 3 ; i < paylen - 1 ; i++)
	c = (c + buf [i]) & 0xff ;
    return 0xff - c ;
}

l2addr *l2net_802154::bcastaddr (void)
{
    return &l2addr_802154_broadcast ;
}

/******************************************************************************
 * Receive data
 */

pktype_t l2net_802154::recv (l2addr **saddr, void *data, int *len)
{
    pktype_t r ;

    r = PK_NONE ;			// no packet received
    while (r == PK_NONE)
    {
	// try to find an already received packet (in the list)
	r = extract_received_packet (saddr, data, len) ;

	if (r == PK_NONE)
	{
	    // No packet received: waits for a complete frame in the buffer
	    // and store it (them) in the list
	    if (read_complete_frame () == -1)
		break ;				// error
	}
    }

    D ("Received packet (" << *len << " bytes)") ;

    return r ;
}

// Explore frame list to find (and remove) a received packet
pktype_t l2net_802154::extract_received_packet (l2addr **saddr, void *data, int *len)
{
    pktype_t r = PK_NONE ;
    std::list <sos::l2net_802154::frame>::iterator f ;

    f = framelist_.begin () ;
    while (r == PK_NONE && f != framelist_.end ())
    {
	if (f->type == sos::l2net_802154::RX_SHORT)
	{
	    // get source address and convert it to a l2addr_802154 object
	    char txtaddr [6] ;
	    std::snprintf (txtaddr, sizeof txtaddr, "%2x:%2x", 
		BYTE_HIGH (f->rx_short_.saddr), BYTE_LOW (f->rx_short_.saddr)) ;
	    *saddr = new l2addr_802154 (txtaddr) ;

	    // transfer data
	    if (*len >= f->rx_short_.len)
		*len = f->rx_short_.len ;
	    std::memcpy (data, f->rx_short_.data, *len) ;

	    // packet addressed to us?
	    if (f->rx_short_.options & sos::l2net_802154::RX_SHORT_OPT_BROADCAST)
		r = PK_BCAST ;
	    else r = PK_ME ;

	    // remove frame from list
	    // std::list <sos::l2net_802154::frame>::iterator tmp ;
	    // tmp = f ;
	    // tmp++ ;
	    framelist_.erase (f) ;
	    // f = tmp ;
	}
	else f++ ;
    }
    return r ;
}

// Buffer does not contain a complete valid frame. Read a complete frame
int l2net_802154::read_complete_frame (void)
{
    int n = -1 ;
    bool found_at_least_one = false ;

    /*
     * We should never reach the end of this buffer
     */

    while (! found_at_least_one &&
	(n = read (fd_, pbuffer_, BUFLEN - (pbuffer_ - buffer_))) > 0)
    {

	pbuffer_ += n ;

	// we may have read more than one frame:
	// extract them all from the buffer to the list
	while (l2net_802154::is_frame_complete ())
	{
	    found_at_least_one = true ;
	    l2net_802154::extract_frame_to_list () ;
	}
    }

    return n ;				// -1 <=> error
}

/*
 * This function checks to see if the buffer contains a valid API packet.
 * If no valid packet is found, skip junk bytes and re-analyze buffer.
 * Return:
 * - true: a valid frame has been found
 * - false: the buffer still needs some bytes
 */

bool l2net_802154::is_frame_complete (void)
{
    byte *start ;
    bool complete = false ;
    bool invalid = true ;

    start = buffer_ ;			// current start of buffer
    while (invalid)
    {
	int buflen ;
	int framelen ;
	int pktlen ;
	int cksum ;
	int i ;

	buflen = pbuffer_ - start ;

	// discard junk bytes before API start delimiter (start-byte)
	i = 0 ;
	while (i < buflen && start [i] != XBEE_START)
	    i++ ;
	// here, i is the relative (to start) position of the start-byte
	// or i >= buflen if not found

	// has a start-byte been found?
	if (i >= buflen)		// no
	{
	    pbuffer_ = buffer_ ;	// reset buffer to empty
	    break ;			// still not complete
	}

	// align start byte at the buffer start
	if (buffer_ != start + i)
	{
	    buflen -= i ;
	    std::memcpy (buffer_, start + i, buflen) ;
	    pbuffer_ = buffer_ + buflen ;
	}

	// do we have enough stuff to extract frame length?
	if (buflen < XBEE_MIN_FRAME_SIZE)
	    break ;			// still not complete

	// does frame length seem realistic ?
	framelen = (buffer_ [1] << 8) | buffer_ [2] ;
	if (framelen > XBEE_MAX_FRAME_SIZE)
	{
	    // skip start-byte
	    start = buffer_ + 1 ;
	    continue ;
	}
	    
	// is checksum already inside the buffer?
	pktlen = framelen + 4 ;
	if (pktlen > buflen)
	    break ;			// still not complete

	// compare checksum
	cksum = compute_checksum (buffer_) ;
	if (cksum != buffer_ [pktlen - 1])
	{
	    // skip start-byte
	    start = buffer_ + 1 ;
	    continue ;
	}

	// if we get there, a valid frame has been found
	invalid = false ;
	complete = true ;
    }

    return complete ;
}

void l2net_802154::extract_frame_to_list (void)
{
    int framelen ;
    int pktlen ;
    struct frame f ;
    
    framelen = (buffer_ [1] << 8) | buffer_ [2] ;
    pktlen = framelen + 4 ;

    f.type = (enum frame_type) buffer_ [3] ;
    switch (buffer_ [3])
    {
	case l2net_802154::TX_STATUS :
	    f.tx_status_.frame_id = buffer_ [4] ;
	    f.tx_status_.status = buffer_ [5] ;
	    break ;
	case l2net_802154::RX_SHORT :
	    f.rx_short_.saddr = (buffer_ [4] << 8) | buffer_ [5] ;
	    f.rx_short_.len = framelen - 5 ;
	    std::memcpy (f.rx_short_.data, buffer_ + 8, f.rx_short_.len) ;
	    f.rx_short_.rssi = buffer_ [6] ;
	    f.rx_short_.options = buffer_ [7] ;
	    break ;
	default :
	    std::cerr << "PKT API " << (int) buffer_ [3] << " unrecognized\n" ;
	    break ;
    }
    framelist_.push_back (f) ;

    // remove extracted packet from buffer
    pbuffer_ -=  pktlen ;
    if (pbuffer_ - buffer_ > 0)		// buffer still contains more bytes
	std::memcpy (buffer_, buffer_ + pktlen, pbuffer_ - buffer_) ;
}

}					// end of namespace sos
