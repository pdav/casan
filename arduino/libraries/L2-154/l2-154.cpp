#include "l2-154.h"

/*
 * 802.15.4 classes for SOS
 *
 * Needs : ZigMsg functions from associated library
 */

l2addr_154 l2addr_154_broadcast ("ff:ff") ;

addr2_t addr2_broadcast = CONST16 (0xff, 0xff) ;

#define	I154_MTU		127


/******************************************************************************
 * l2addr_154 methods
 */

// constructor
l2addr_154::l2addr_154 ()
{
}

// constructor:
l2addr_154::l2addr_154 (const char *a)
{
    int i = 0 ;
    byte b = 0 ;
    byte buf [I154ADDRLEN] ;

    /*
     * General loop, when 8-byte addresses will be supported
     */

    while (*a != '\0' && i < I154ADDRLEN)
    {
	if (*a == ':')
	{
	    buf [i++] = b ;
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
	    for (i = 0 ; i < I154ADDRLEN ; i++)
		buf [i] = 0 ;
	    break ;
	}
	a++ ;
    }
    if (i < I154ADDRLEN)
	buf [i] = b ;

    addr_ = CONST16 (buf [1], buf [0]) ;
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

#if 0
// Raw MAC address (array of 2 bytes)
bool l2addr_154::operator!= (const unsigned char *other)
{
    return memcmp (this->addr_, other, I154ADDRLEN) != 0 ;
}
#endif

#if 0
void l2addr_154::set_raw_addr (const unsigned char *mac_addr) 
{
    memcpy (this->addr_, mac_addr, I154ADDRLEN) ;
}

unsigned char * l2addr_154::get_raw_addr (void) 
{
    return this->addr_ ;
}
#endif

void l2addr_154::print (void) {
    Serial.print (F ("802.15.4 : \033[32m")) ;
    Serial.print (BYTE_HIGH (addr_), HEX) ;
    Serial.print (':') ;
    Serial.print (BYTE_LOW (addr_), HEX) ;
#if 0
    for (int i = 0 ; i < I154ADDRLEN ; i++)
    {
	if (i > 0)
	    Serial.print (':') ;
	Serial.print (addr_ [i], HEX) ;
    }
#endif
    Serial.print (F ("\033[00m")) ;
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

void l2net_154::start (l2addr *a, bool promisc, size_t mtu, channel_t chan, panid_t panid)
{
    myaddr_ = ((l2addr_154 *) a)->addr_ ;
    ZigMsg.addr2 (myaddr_) ;
    ZigMsg.channel (chan) ;
    ZigMsg.panid (panid) ;
    ZigMsg.promiscuous (false) ;

    if (mtu == 0 || mtu > I154_MTU)
	mtu = I154_MTU ;		// excluding MAC header
    mtu_ = mtu ;

    curframe_ = NULL ;			// no currently received frame

    ZigMsg.start () ;
}

/*
 * Send packet
 *
 * Returns true if data is sent.
 */

bool l2net_154::send (l2addr &dest, const uint8_t *data, size_t len) 
{
    return ZigMsg.sendto (((l2addr_154 *) &dest)->addr_, len, data) ;
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

l2_recv_t l2net_154::recv (void) 
{
    l2_recv_t r ;

    if (curframe_ != NULL)
	ZigMsg.skip_received () ;

    curframe_ = ZigMsg.get_received () ;
    if (curframe_ != NULL
	    && curframe_->frametype == Z_FT_DATA
	    && Z_GET_DST_ADDR_MODE (curframe_->fcf) == Z_ADDRMODE_ADDR2
	    && Z_GET_SRC_ADDR_MODE (curframe_->fcf) == Z_ADDRMODE_ADDR2
	    && Z_GET_INTRA_PAN (curframe_->fcf)
	    )
    {
	if (curframe_->dstaddr != myaddr_ && curframe_->dstaddr != addr2_broadcast)
	    r = L2_RECV_WRONG_DEST ;
	else
	    r = L2_RECV_RECV_OK ;
    }
    else r = L2_RECV_EMPTY ;

    return r ;
}

l2addr *l2net_154::bcastaddr (void)
{
    return &l2addr_154_broadcast ;
}

void l2net_154::get_src (l2addr *mac) 
{
    l2addr_154 *a = (l2addr_154 *) mac ;
    a->addr_ = curframe_->srcaddr ;
}

void l2net_154::get_dst (l2addr *mac) 
{
    l2addr_154 *a = (l2addr_154 *) mac ;
    a->addr_ = curframe_->dstaddr ;
}

l2addr *l2net_154::get_src (void)
{
    l2addr_154 *a = new l2addr_154 ;
    get_src (a) ;
    return a ;
}

l2addr *l2net_154::get_dst (void)
{
    l2addr_154 *a = new l2addr_154 ;
    get_dst (a) ;
    return a ;
}

uint8_t *l2net_154::get_payload (int offset) 
{
    return curframe_->payload ;
}

size_t l2net_154::get_paylen (void) 
{
    return curframe_->paylen ;
}

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
