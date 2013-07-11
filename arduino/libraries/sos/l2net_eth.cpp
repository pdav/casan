#include "l2net_eth.h"

l2net_eth::l2net_eth(l2addr *mac_addr, int mtu, int ethtype)
	: _s(0), _eth_type (ethtype)
{
	W5100.init();
	W5100.writeSnMR(_s, SnMR::MACRAW);
	W5100.execCmdSn(_s, Sock_OPEN);
	_mac_addr = (l2addr_eth *) mac_addr ;
	W5100.setMACAddress(_mac_addr->get_raw_addr());
	mtu_ = mtu ;
	_rbuf = (byte *) malloc (mtu_) ;
}

l2net_eth::~l2net_eth() 
{
	if(_mac_addr != NULL)
		delete _mac_addr;
	if(_master_addr != NULL)
		delete _master_addr;
	free (_rbuf) ;
}

int l2net_eth::available() 
{
	_rbuflen = W5100.getRXReceivedSize(_s);
	return _rbuflen ; 
}

size_t l2net_eth::send(l2addr &mac_dest, const uint8_t *data, size_t len) 
{
	l2addr_eth * m = (l2addr_eth *) &mac_dest;
	byte *sbuf = NULL;
	int sbuflen;
	size_t reste;

	// to include the two size bytes
	len += 2;

	reste = mtu_ < len ? len - mtu_ - OFFSET_DATA: 0;
	sbuflen = mtu_ < len + OFFSET_DATA ? mtu_ : len + OFFSET_DATA;
	sbuf = (byte*) malloc(sbuflen);

	memcpy(sbuf + OFFSET_DEST_ADDR, m->get_raw_addr(), 6);
	memcpy(sbuf + OFFSET_SRC_ADDR, _mac_addr->get_raw_addr(), 6);
	{
		sbuf[OFFSET_ETHTYPE   ] = (char) (_eth_type >> 8) & 0xFF;
		sbuf[OFFSET_ETHTYPE +1] = (char) _eth_type & 0xFF;
	}

	// message size
	sbuf [OFFSET_SIZE   ] = BYTE_HIGH (len) ;
	sbuf [OFFSET_SIZE +1] = BYTE_LOW  (len) ;

	memcpy(sbuf + OFFSET_DATA, data, len);

	W5100.send_data_processing(_s, sbuf, sbuflen);
	W5100.execCmdSn(_s, Sock_SEND_MAC);

	free(sbuf);
	return reste;
}

/*
 * returns L2_RECV_WRONG_SENDER if it's not the master the sender;
 * returns L2_RECV_WRONG_DEST if the dest is wrong 
	 * (not the good mac address or the broadcast address)
 * returns L2_RECV_WRONG_ETHTYPE if it's the wrong eth type
 * returns L2_RECV_TRUNCATED if packet is too large (i.e. has been truncated)
 * returns L2_RECV_RECV_OK if ok
 */
l2_recv_t l2net_eth::recv(void) 
{
	size_t l ;
	if ( available() ) {
		l2_recv_t ret = check_received();

		if (ret != L2_RECV_RECV_OK ) {
			return ret;
		}

		l = get_payload_length() ;
		return L2_RECV_RECV_OK;
	}
	return L2_RECV_EMPTY;
}

l2_recv_t l2net_eth::check_received(void) 
{
	bool truncated = false;
	if(_rbuflen > mtu_) 
	{
		truncated = true;
		_rbuflen = mtu_;
	}

	// copy received packet from W5100 internal buffer to instance
	// private variable
	W5100.recv_data_processing(_s, _rbuf, _rbuflen);
	W5100.execCmdSn(_s, Sock_RECV);

	// There is an offset in the reception (2 bytes, see the w5100 datasheet)
	byte * packet = _rbuf + OFFSET_RECEPTION;
	// we check the source address (only the master must send informations)
	// TODO : we don't use it for now, but we will use this feature later
	PRINT_DEBUG_STATIC("JUSTE AVANT DE TESTER");
	if(_master_addr != NULL)
	{
		((l2addr_eth *) _master_addr)->print();
	}

	if(_master_addr != NULL && 
			! (*((l2addr_eth *) _master_addr) == (l2addr_eth) l2addr_eth_broadcast) &&
			*_master_addr != (packet + OFFSET_SRC_ADDR) ) 
	{
		PRINT_DEBUG_STATIC("\033[31m\tRecv : don't come from master\033[00m");
		return L2_RECV_WRONG_SENDER;
	}

	// we check the destination address
	if( _mac_addr != NULL && *_mac_addr != (packet +OFFSET_DEST_ADDR)) 
	{
		// Serial.print(F("\033[36m\tRecv : not explicitly for us\033[00m"));
		if(l2addr_eth_broadcast != (packet + OFFSET_DEST_ADDR)) {
			// PRINT_DEBUG_STATIC("\033[31m : not even broadcast\033[00m");
			return L2_RECV_WRONG_DEST;
		}
		// PRINT_DEBUG_STATIC("\033[32m : broadcast\033[00m ");
	}
	// we check the ethernet type

	{
		uint8_t a = *(packet + OFFSET_ETHTYPE);
		uint8_t b = *(packet + OFFSET_ETHTYPE + 1);
		if( a != (uint8_t) (_eth_type >> 8) || 
				b != (uint8_t) (_eth_type & 0xFF)) {
			return L2_RECV_WRONG_ETHTYPE;
		}
	}

	// if this is a real coap message
	if(truncated || get_payload_length() > mtu_)
	{
		return L2_RECV_TRUNCATED;
	}
	return L2_RECV_RECV_OK;
}

l2addr *l2net_eth::bcastaddr (void)
{
	return &l2addr_eth_broadcast;
}

void l2net_eth::get_mac_src(l2addr * mac_src) 
{
	l2addr_eth * m = (l2addr_eth *) mac_src;
	m->set_addr( _rbuf + OFFSET_RECEPTION + OFFSET_SRC_ADDR );
}

uint8_t * l2net_eth::get_offset_payload(int offset) 
{
	uint8_t off = OFFSET_RECEPTION + OFFSET_DATA + offset;
	return _rbuf + off;
}

// we add 2 bytes to know the real payload
size_t l2net_eth::get_payload_length(void) 
{
	size_t rlen = 0;
	rlen = (*(_rbuf + OFFSET_RECEPTION + OFFSET_SIZE) << 8) & 0xFF00;
	rlen = rlen | (*(_rbuf + OFFSET_RECEPTION + OFFSET_SIZE + 1) & 0xFF);
	return rlen;
}

void l2net_eth::set_master_addr(l2addr * master_addr) 
{
	_master_addr = (l2addr_eth *) master_addr;
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
	Serial.print (F(" bytes ") );

	print_eth_addr (pkt + OFFSET_SRC_ADDR) ;
	Serial.print (F("->") );
	print_eth_addr (pkt + OFFSET_DEST_ADDR) ;

	w = * (word *) (pkt + OFFSET_ETHTYPE) ;
	Serial.print (F(" Ethtype: ") );
	Serial.print (w, HEX) ;

	Serial.print (" ") ;

	for (int z = 0 ; z < len ; z++)
	{
		Serial.print (' ') ;
		Serial.print (pkt [z + OFFSET_ETHTYPE + 2], HEX) ;
	}
	Serial.println () ;
}
