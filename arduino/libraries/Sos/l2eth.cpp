#include "l2eth.h"

l2addr_eth l2addr_eth_broadcast ("ff:ff:ff:ff:ff:ff") ;

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
l2addr_eth & l2addr_eth::operator = (const l2addr_eth &x)
{
    if (addr_ == x.addr_)
	return *this ;
    memcpy (addr_, x.addr_, ETHADDRLEN) ;
    return *this ;
}

/******************************************************************************
* l2addr_eth methods
*/

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

bool l2addr_eth::operator!= (const unsigned char* mac_addr)
{
    return memcmp (this->addr_, mac_addr, ETHADDRLEN) != 0 ;
}

void l2addr_eth::set_addr (const unsigned char* mac_addr) 
{
    memcpy (this->addr_, mac_addr, ETHADDRLEN) ;
}
unsigned char * l2addr_eth::get_raw_addr (void) 
{
    return this->addr_ ;
}

void l2addr_eth::print () {
    Serial.print (F ("IP : ")) ;
    for (int i (0) ; i < ETHADDRLEN ; i++) {
	Serial.print ("\033[32m") ;
	Serial.print (addr_[i], HEX) ;
	if (i != ETHADDRLEN - 1)
	    Serial.print (':') ;
    }
    Serial.println ("\033[00m") ;
}
























l2net_eth::l2net_eth (l2addr *mac_addr, size_t mtu, int ethtype)
	: sock_ (0), ethtype_ (ethtype)
{
    W5100.init () ;
    W5100.writeSnMR (sock_, SnMR::MACRAW) ;
    W5100.execCmdSn (sock_, Sock_OPEN) ;
    mac_addr_ = (l2addr_eth *) mac_addr ;
    W5100.setMACAddress (mac_addr_->get_raw_addr ()) ;
    mtu_ = mtu ;
    rbuf_ = (byte *) malloc (mtu_) ;
    master_addr_ = NULL ;
}

l2net_eth::~l2net_eth () 
{
    if (mac_addr_ != NULL)
	delete mac_addr_ ;
    if (master_addr_ != NULL)
	delete master_addr_ ;
    free (rbuf_) ;
}

int l2net_eth::available () 
{
    rbuflen_ = W5100.getRXReceivedSize (sock_) ;
    return rbuflen_ ; 
}

size_t l2net_eth::send (l2addr &mac_dest, const uint8_t *data, size_t len) 
{
    l2addr_eth *m = (l2addr_eth *) &mac_dest ;
    byte *sbuf = NULL ;
    int sbuflen ;
    size_t reste ;

    // to include the two size bytes
    len += 2 ;

    reste = mtu_ < len ? len - mtu_ - OFFSET_DATA : 0 ;
    sbuflen = mtu_ < len + OFFSET_DATA ? mtu_ : len + OFFSET_DATA ;
    sbuf = (byte *) malloc (sbuflen) ;

    memcpy (sbuf + OFFSET_DEST_ADDR, m->get_raw_addr (), 6) ;
    memcpy (sbuf + OFFSET_SRC_ADDR, mac_addr_->get_raw_addr (), 6) ;

    sbuf [OFFSET_ETHTYPE    ] = (char) ((ethtype_ >> 8) & 0xff) ;
    sbuf [OFFSET_ETHTYPE +1 ] = (char) ((ethtype_     ) & 0xff) ;

    // message size
    sbuf [OFFSET_SIZE    ] = BYTE_HIGH (len) ;
    sbuf [OFFSET_SIZE + 1] = BYTE_LOW  (len) ;

    memcpy (sbuf + OFFSET_DATA, data, len) ;

    W5100.send_data_processing (sock_, sbuf, sbuflen) ;
    W5100.execCmdSn (sock_, Sock_SEND_MAC) ;

    free (sbuf) ;
    return reste ;
}

/*
* returns L2_RECV_WRONG_SENDER if it's not the master the sender ;
* returns L2_RECV_WRONG_DEST if the dest is wrong 
     * (not the good mac address or the broadcast address)
* returns L2_RECV_WRONG_ETHTYPE if it's the wrong eth type
* returns L2_RECV_TRUNCATED if packet is too large (i.e. has been truncated)
* returns L2_RECV_RECV_OK if ok
*/
l2_recv_t l2net_eth::recv (void) 
{
    l2_recv_t r ;

    r = L2_RECV_EMPTY ;
    if (available ())
	r = check_received () ;
    return r ;
}

l2_recv_t l2net_eth::check_received (void) 
{
    bool truncated = false ;
    byte *packet ;

    if (rbuflen_ > mtu_) 
    {
	truncated = true ;
	rbuflen_ = mtu_ ;
    }

    // copy received packet from W5100 internal buffer to instance
    // private variable
    W5100.recv_data_processing (sock_, rbuf_, rbuflen_) ;
    W5100.execCmdSn (sock_, Sock_RECV) ;

    // There is an offset in the reception (2 bytes, see the w5100 datasheet)
    packet = rbuf_ + OFFSET_RECEPTION ;
    // Check the source address (only the master must send informations)
    // TODO : we don't use it for now, but we will use this feature later
    PRINT_DEBUG_STATIC ("JUSTE AVANT DE TESTER") ;
    if (master_addr_ != NULL)
	((l2addr_eth *) master_addr_)->print () ;

    if (master_addr_ != NULL && 
	*(l2addr_eth *) master_addr_ != (l2addr_eth) l2addr_eth_broadcast &&
	*master_addr_ != packet + OFFSET_SRC_ADDR) 
    {
	PRINT_DEBUG_STATIC ("\033[31m\tRecv : don't come from master\033[00m") ;
	return L2_RECV_WRONG_SENDER ;
    }

    // Check the destination address
    if (mac_addr_ != NULL && *mac_addr_ != (packet +OFFSET_DEST_ADDR)) 
    {
	// Serial.print (F ("\033[36m\tRecv : not explicitly for us\033[00m")) ;
	if (l2addr_eth_broadcast != (packet + OFFSET_DEST_ADDR))
	{
	    // PRINT_DEBUG_STATIC ("\033[31m : not even broadcast\033[00m") ;
	    return L2_RECV_WRONG_DEST ;
	}
	// PRINT_DEBUG_STATIC ("\033[32m : broadcast\033[00m ") ;
    }

    // Check the ethernet type
    if (packet [OFFSET_ETHTYPE] != (uint8_t) (ethtype_ >> 8) || 
		packet [OFFSET_ETHTYPE + 1] != (uint8_t) (ethtype_ & 0xff))
    {
	return L2_RECV_WRONG_ETHTYPE ;
    }

    // if this is a real coap message
    if (truncated || get_payload_length () > mtu_)
    {
	return L2_RECV_TRUNCATED ;
    }
    return L2_RECV_RECV_OK ;
}

l2addr *l2net_eth::bcastaddr (void)
{
    return &l2addr_eth_broadcast ;
}

void l2net_eth::get_mac_src (l2addr *mac_src) 
{
    l2addr_eth *m = (l2addr_eth *) mac_src ;
    m->set_addr (rbuf_ + OFFSET_RECEPTION + OFFSET_SRC_ADDR) ;
}

uint8_t *l2net_eth::get_offset_payload (int offset) 
{
    uint8_t off ;
    
    off = OFFSET_RECEPTION + OFFSET_DATA + offset ;
    return rbuf_ + off ;
}

// we add 2 bytes to know the real payload
size_t l2net_eth::get_payload_length (void) 
{
    size_t rlen ;

    rlen = (*(rbuf_ + OFFSET_RECEPTION + OFFSET_SIZE) << 8) & 0xff00 ;
    rlen = rlen | (*(rbuf_ + OFFSET_RECEPTION + OFFSET_SIZE + 1) & 0xff) ;
    return rlen ;
}

void l2net_eth::set_master_addr (l2addr * master_addr) 
{
    master_addr_ = (l2addr_eth *) master_addr ;
}

void l2net_eth::print_eth_addr (byte addr []) 
{
    int i ;
    for (i = 0 ; i < 6 ; i++)
    {
	if (i > 0)
		Serial.print (':') ;
	Serial.print (addr [i], HEX) ;
    }
}

void l2net_eth::print_packet (byte pkt [], int len) 
{
    word w ;

    w = * (word *) pkt ;
    Serial.print (len) ;
    Serial.print (F (" bytes ") ) ;

    print_eth_addr (pkt + OFFSET_SRC_ADDR) ;
    Serial.print (F ("->") ) ;
    print_eth_addr (pkt + OFFSET_DEST_ADDR) ;

    w = * (word *) (pkt + OFFSET_ETHTYPE) ;
    Serial.print (F (" Ethtype: ") ) ;
    Serial.print (w, HEX) ;

    Serial.print (" ") ;

    for (int z = 0 ; z < len ; z++)
    {
	Serial.print (' ') ;
	Serial.print (pkt [z + OFFSET_ETHTYPE + 2], HEX) ;
    }
    Serial.println () ;
}
