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
	_rbuflen = W5100.getRXReceivedSize(_s) ;
	return _rbuflen ; //get_payload_length();
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
		long eth_type = htonl(_eth_type);

		memcpy(sbuf + OFFSET_ETHTYPE, &eth_type, 2);
		//memcpy(sbuf + OFFSET_ETHTYPE +1, &_eth_type, 1);
	}
	memcpy(sbuf + OFFSET_DATA, data, len);

	W5100.send_data_processing(_s, sbuf, sbuflen);
	W5100.execCmdSn(_s, Sock_SEND_MAC);
	//Serial.print(F("Envoi : "));
	//print_packet(sbuf, sbuflen);

	free(sbuf);
	return reste;
}

/*
 * returns 1 if it's not the master the sender;
 * returns 2 if the dest is wrong (not the good mac address or the broadcast address)
 * returns 3 if it's the wrong eth type
 * return 0 if ok
 */
uint8_t EthernetRaw::recv(void) {
	return recv(NULL, NULL);
}

uint8_t EthernetRaw::recv(uint8_t *data, int *len) {
	Serial.println(F("Recv !"));
	PRINT_FREE_MEM
	W5100.recv_data_processing(_s, _rbuf, _rbuflen);
	W5100.execCmdSn(_s, Sock_RECV);
	// There is an offset in the reception (2 bytes, see the w5100 datasheet)
	byte * packet = _rbuf + OFFSET_RECEPTION;
	// we check the source address (only the master must send informations)
	// TODO : we don't use it for now, but we will use this feature later
	if(_master_addr != NULL && *_master_addr != packet + OFFSET_SRC_ADDR ) {
		Serial.println(F("Recv : ne provient pas du master"));
		PRINT_FREE_MEM
		return 1;
	}
	// we check the destination address
	if( *_mac_addr != packet +OFFSET_DEST_ADDR) {
		Serial.println(F("Recv : ne nous est pas destiné"));
		if(l2addr_eth_broadcast != packet + OFFSET_DEST_ADDR) {
			Serial.println(F("\033[31m Recv : not even broadcast \033[00m "));
			PRINT_FREE_MEM
			return 2;
		}
	}
	// we check the ethernet type
	if(memcmp(packet + OFFSET_ETHTYPE, &_eth_type, 2) != 0) {
		Serial.println(F("Recv : ethtype"));
		PRINT_FREE_MEM
		return 3;
	}
	if(data != NULL) {
		*len = _rbuflen - OFFSET_RECEPTION - OFFSET_DATA;
		memcpy(data, packet + OFFSET_DATA, *len);
	}
	//Serial.println(F("Reçoi : "));
	//print_packet(packet, _rbuflen);
	return 0;
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

// There is an offset in the reception (2 bytes, see the w5100 datasheet)
int EthernetRaw::get_payload_length(void) {
	return _rbuflen - (OFFSET_DATA + OFFSET_RECEPTION);
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
