#include "ethernetraw.h"

EthernetRaw::EthernetRaw() : _s(0) {//_sock(MAX_SOCK_NUM)
	W5100.init();
	W5100.writeSnMR(_s, SnMR::MACRAW);
	W5100.execCmdSn(_s, Sock_OPEN);
}

EthernetRaw::~EthernetRaw() {
	if(_mac_addr != NULL)
		delete _mac_addr;
	if(_master_addr != NULL)
		delete _master_addr;
}

int EthernetRaw::available() {
	_rbuflen = W5100.getRXReceivedSize(_s);
	return _rbuflen ; 
}

size_t EthernetRaw::send(l2addr &mac_dest, uint8_t data) {
	return send(mac_dest, &data, 1);
}

size_t EthernetRaw::send(l2addr &mac_dest, const uint8_t *data, size_t len) {
	l2addr_eth * m = (l2addr_eth *) &mac_dest;
	byte *sbuf = NULL;
	int sbuflen;
	size_t reste;

	reste = BUFFER_SIZE < len ? len - BUFFER_SIZE - OFFSET_DATA: 0;
	sbuflen = BUFFER_SIZE < len + OFFSET_DATA ? BUFFER_SIZE : len + OFFSET_DATA;
	sbuf = (byte*) malloc(sbuflen);

	memcpy(sbuf + OFFSET_DEST_ADDR, m->get_raw_addr(), 6);
	memcpy(sbuf + OFFSET_SRC_ADDR, _mac_addr->get_raw_addr(), 6);
	{
		*(sbuf + OFFSET_ETHTYPE   ) = (char) _eth_type >> 8;
		*(sbuf + OFFSET_ETHTYPE +1) = (char) _eth_type & 0xFF;
	}

	// message size
	sbuf [OFFSET_SIZE] = BYTE_HIGH (len) ;
	sbuf [OFFSET_SIZE] = BYTE_LOW  (len) ;

	memcpy(sbuf + OFFSET_DATA, data, len);

	W5100.send_data_processing(_s, sbuf, sbuflen);
	W5100.execCmdSn(_s, Sock_SEND_MAC);
	//Serial.print(F("Envoi : "));
	//print_packet(sbuf, sbuflen);

	free(sbuf);
	return reste;
}

/*
 * returns ETH_RECV_WRONG_SENDER if it's not the master the sender;
 * returns ETH_RECV_WRONG_DEST if the dest is wrong 
	 * (not the good mac address or the broadcast address)
 * returns ETH_RECV_WRONG_ETHTYPE if it's the wrong eth type
 * return ETH_RECV_RECV_OK if ok
 */
eth_recv_t EthernetRaw::recv(void) {
	return recv(NULL, NULL);
}

eth_recv_t EthernetRaw::recv(uint8_t *data, size_t *len) {
	PRINT_DEBUG_STATIC("Recv !");
	if ( available() ) {
		eth_recv_t ret = check_received();

		if (ret != ETH_RECV_RECV_OK ) {
			return ret;
		}

		if(data != NULL) {
			*len = get_payload_length();
			memcpy(data, _rbuf + OFFSET_RECEPTION + OFFSET_DATA, *len);
		}
	} else {
		return ETH_RECV_EMPTY;
	}
	return ETH_RECV_RECV_OK;
}

eth_recv_t EthernetRaw::check_received(void) {
	W5100.recv_data_processing(_s, _rbuf, _rbuflen);
	W5100.execCmdSn(_s, Sock_RECV);
	// There is an offset in the reception (2 bytes, see the w5100 datasheet)
	byte * packet = _rbuf + OFFSET_RECEPTION;
	// we check the source address (only the master must send informations)
	// TODO : we don't use it for now, but we will use this feature later
	if(_master_addr != NULL && *_master_addr != packet + OFFSET_SRC_ADDR ) {
		PRINT_DEBUG_STATIC("Recv : ne provient pas du master");
		return ETH_RECV_WRONG_SENDER;
	}
	// we check the destination address
	if( *_mac_addr != (packet +OFFSET_DEST_ADDR)) {
		PRINT_DEBUG_STATIC("Recv : ne nous est pas destiné");
		if(l2addr_eth_broadcast != (packet + OFFSET_DEST_ADDR)) {
			PRINT_DEBUG_STATIC("\033[31m Recv : not even broadcast \033[00m");
			return ETH_RECV_WRONG_DEST;
		}
		PRINT_DEBUG_STATIC("\033[32m Recv : broadcast = ok \033[00m ");
	}
	// we check the ethernet type

	{
		uint8_t a = *(packet + OFFSET_ETHTYPE);
		uint8_t b = *(packet + OFFSET_ETHTYPE + 1);
		if( a != (uint8_t) (_eth_type >> 8) || 
				b != (uint8_t) (_eth_type & 0xFF)) {
			PRINT_DEBUG_STATIC("\033[31m Recv : ethtype\033[00m, received : ");
			Serial.println(a, HEX);
			Serial.println(b, HEX);
			return ETH_RECV_WRONG_ETHTYPE;
		}
	}
	//PRINT_DEBUG_STATIC("Reçoi : ");
	//print_packet(packet, _rbuflen);
	return ETH_RECV_RECV_OK;
}

void EthernetRaw::get_mac_src(l2addr * mac_src) {
	l2addr_eth * m = (l2addr_eth *) mac_src;
	m->set_addr( _rbuf + OFFSET_RECEPTION + OFFSET_SRC_ADDR );
}

uint8_t * EthernetRaw::get_offset_payload(int offset) {
	uint8_t off = OFFSET_RECEPTION + OFFSET_DATA + offset;
	//Serial.print(F("OFFSET : "));
	//Serial.println(off);
	return _rbuf + off;
}

// we add 2 bytes to know the real payload
size_t EthernetRaw::get_payload_length(void) {
	size_t rlen = 0;
	// memcpy(&rlen, _rbuf + OFFSET_SIZE, 2);
	rlen = *(_rbuf + OFFSET_RECEPTION + OFFSET_SIZE) << 8 & 0xFF00;
	rlen = rlen | (*(_rbuf + OFFSET_RECEPTION + OFFSET_SIZE + 1) & 0xFF);
	return rlen;
}

void EthernetRaw::set_master_addr(l2addr * master_addr) {
	if(_master_addr != NULL)
		delete _master_addr;
	_master_addr = (l2addr_eth *) master_addr;
}

void EthernetRaw::set_mac(l2addr *mac_address) {
	_mac_addr = (l2addr_eth *) mac_address;
	W5100.setMACAddress(_mac_addr->get_raw_addr());
	/*
	   Serial.print(F("mac : "));
	   for(int i = 0 ; i < 6 ; i++)
	   {
	   Serial.print(_mac_addr[i]);
	   }
	   Serial.println();
	   */
}

void EthernetRaw::set_ethtype(int eth_type) {
	_eth_type = eth_type;
}

void EthernetRaw::print_eth_addr (byte addr []) {
	int i ;
	for (i = 0 ; i < 6 ; i++)
	{
		if (i > 0)
			Serial.print (':') ;
		Serial.print (addr [i], HEX) ;
	}
}

void EthernetRaw::print_packet (byte pkt [], int len) {
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

	for (int z = 0 ; z < 10 ; z++)
	{
		Serial.print (' ') ;
		Serial.print (pkt [z + OFFSET_ETHTYPE + 2], HEX) ;
	}
	Serial.println () ;
}
