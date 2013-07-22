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
	    std::cout << "J'écris " << buf << "\n" ;
	    write (fd_, buf, strlen (buf)) ;
	    write (fd_, "ATMY\r", 5) ;
#if 0
	    sleep (1) ;
	    write (fd_, "ATDLcafe\r", 9) ;
	    sleep (1) ;
	    // write (fd_, "ATDH0\r", 5) ;
	    sleep (1) ;
	    write (fd_, "ATMY\r", 5) ;
	    sleep (1) ;
	    write (fd_, "ATDH\r", 5) ;
	    sleep (1) ;
	    write (fd_, "ATDL\r", 10) ;
	    sleep (1) ;
	    write (fd_, "ATCN\r", 5) ;
	    sleep (1) ;
	    write (fd_, "tagada\r", 7) ;
#endif
	}

	// enter API mode
	sleep (1) ;
	write (fd_, "ATAP1\r", 6) ;
	sleep (1) ;
	write (fd_, "ATCN\r", 5) ;
	
	n = 0 ;
    }

    return n ;
}

void l2net_802154::term (void)
{
    close (fd_) ;
}

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
for (int i = 0 ; i < 5 ; i++) {
	if (encode_transmit (cmd, cmdlen, da, (byte *) data, len))
	    n = write (fd_, cmd, cmdlen) ;
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

    *b++ = compute_checksum (cmd, cmdlen) ;

    std::cout << "PKT=" ;
    for (int i = 0 ; i < cmdlen ; i++)
    {
	if (i > 0)
	    std::cout << ":" ;
	std::cout << std::hex << (int) cmd [i] ;
    }
    std::cout << "\n" ;

    return true ;
}

// buf = encoded frame, paylen = len of encoded frame (including checksum)
int l2net_802154::compute_checksum (const byte *buf, int paylen)
{
    int c = 0 ;

    for (int i = 3 ; i < paylen - 1 ; i++)
	c = (c + buf [i]) & 0xff ;
    return 0xff - c ;
}

l2addr *l2net_802154::bcastaddr (void)
{
    return &l2addr_802154_broadcast ;
}

pktype_t l2net_802154::recv (l2addr **saddr, void *data, int *len)
{
    byte buf [MAXBUF] ;
    char buf2 [MAXBUF] ;
    int n ;

    while ((n = read (fd_, buf, sizeof buf)) != -1)
    {
	char linehex [MAXBUF], *phex, lineascii [MAXBUF], *pascii ;
	int i ;

	phex = linehex ;
	pascii = lineascii ;
	for (i = 0 ; i < n ; i++)
	{
	    int c ;

	    if (i % 16 == 0 && (phex != linehex))
	    {
		*phex = '\0' ;
		*pascii = '\0' ;
		std::sprintf (buf2, "%05d %-52.52s  %-20.20s", i-16, linehex, lineascii) ;
		std::cout << buf2 << "\n" ;
		phex = linehex ;
		pascii = lineascii ;
	    }

	    if (i % 16 != 0 && i % 4 == 0)
	    {
		*phex++ = ' ' ;
		*pascii++ = ' ' ;
	    }

	    c = (buf [i] & 0xf0) >> 4 ;
	    *phex++ = (c > 9) ? (c-10) + 'a' : c + '0' ;
	    c = (buf [i] & 0x0f) ;
	    *phex++ = (c > 9) ? (c-10) + 'a' : c + '0' ;
	    *phex++ = ' ' ;

	    if (isascii (buf [i]) && isgraph (buf [i]))
		*pascii++ = buf [i] ;
	    else *pascii++ = '.' ;
	}
	if (i % 16 != 0)
	{
	    *phex = '\0' ;
	    *pascii = '\0' ;
	    std::sprintf (buf2, "%05d %-52.52s  %-20.20s", (i / 16) * 16, linehex, lineascii) ;
	    std::cout << buf2 << "\n" ;
	}
    }
}

}					// end of namespace sos
