#include "coap.h"

Coap::Coap(l2addr *mac_addr, uint8_t *eth_type) {
	_eth = new EthernetRaw();
	_eth->set_mac(mac_addr);
	_eth->set_ethtype(eth_type);
}

Coap::Coap(EthernetRaw *e) {
	_eth = e;
}

Coap::Coap() {
}

Coap::~Coap(void) {
	delete _eth;
}

void Coap::set_l2(EthernetRaw *e) {
	if( _eth != NULL )
		delete _eth;
	_eth = e;
}

void Coap::send(l2addr &mac_addr_dest, Message &m) {
	uint8_t sbuf[BUFFER_SIZE] = {0};
	sbuf[COAP_OFFSET_TYPE] |= (m.get_type() & 0x3) << 4;
	sbuf[COAP_OFFSET_TKL] |= (m.get_token_length() & 0xF);
	sbuf[COAP_OFFSET_CODE] = m.get_code();
	int id = m.get_id();
	memcpy(sbuf + COAP_OFFSET_ID, &id, 2);
	memcpy(sbuf + COAP_OFFSET_TOKEN, m.get_token(), m.get_token_length());
	{
		uint8_t payload_offset = 0;
		payload_offset = m.get_token_length() + (4 - (m.get_token_length() % 4));
		sbuf[COAP_OFFSET_TOKEN + payload_offset] = 0xFF;
		memcpy(sbuf + COAP_OFFSET_TOKEN + payload_offset + 1, 
				m.get_payload(), (unsigned int) m.get_payload_length());
		_eth->send( mac_addr_dest, sbuf, 
				COAP_OFFSET_TOKEN + payload_offset + m.get_payload_length() +1);
	}
}

// mac_addr of the master or broadcast, the message we get
uint8_t Coap::recv(Message &m) {
	uint8_t ret = fetch();
	if(ret != 0)
		return ret;
	m.set_type(get_type());
	m.set_code(get_code());
	m.set_id(get_id());
	m.set_token(get_token_length(), get_token());
	m.set_payload(get_payload_length(), get_payload());
	m.set_options(get_options_length(), get_options());
	return 0;
}

uint8_t Coap::coap_available() {
	return _eth->available(); // returns the payload
}

/*
 * returns 1 if it's not the master the sender;
 * returns 2 if the dest is wrong (not the good mac address or the broadcast address)
 * returns 3 if it's the wrong eth type
 * return 0 if ok
 */
uint8_t Coap::fetch() {
	return _eth->recv(); 
}

uint8_t Coap::get_type(void) {
	return (uint8_t) ((*(_eth->get_offset_payload(COAP_OFFSET_TYPE))) >> 4 ) & 0x03;
}

uint8_t Coap::get_code(void) {
	return *(_eth->get_offset_payload(COAP_OFFSET_CODE));
}

int Coap::get_id(void) {
	int ret;
	memcpy(&ret, _eth->get_offset_payload(COAP_OFFSET_ID), 2);
	return ret;
}

uint8_t Coap::get_token_length(void) {
	return (*(_eth->get_offset_payload(COAP_OFFSET_TKL))) & 0x0F;
}

uint8_t * Coap::get_token(void) {
	return _eth->get_offset_payload(COAP_OFFSET_TOKEN);
}

uint8_t Coap::get_payload_length(void) {
	// return _eth->get_payload_length() - (payload_offset + COAP_OFFSET_TOKEN + *token_length);
	return 5 ; //_payload_length; FIXME
}

uint8_t * Coap::get_payload(void) {
	uint8_t payload_offset = 0;
	uint8_t tmp = 0;
	uint8_t tkl = get_token_length();
	payload_offset = (tkl == 0 || tkl == 4) ? COAP_OFFSET_TOKEN + tkl : COAP_OFFSET_TOKEN + tkl + (4 - (tkl % 4));
	tmp = *(_eth->get_offset_payload(payload_offset));
	while(tmp != 0xFF && payload_offset < _eth->get_payload_length()) {
		payload_offset++;
		tmp = *(_eth->get_offset_payload(payload_offset)); 
	}
	++payload_offset;
	return _eth->get_offset_payload(payload_offset);
}

uint8_t Coap::get_options_length(void) { // FIXME : It's not the way it works
	uint8_t tkl = get_token_length();
	uint8_t complement = (tkl%4) == 0 ? 0 : 4 - (tkl%4); // TODO : check if we can use xor
	return get_payload() - (get_token() + tkl + complement) - 1; 
}

uint8_t * Coap::get_options(void) { // FIXME : it's not the way it works
	uint8_t options_offset = 0;
	uint8_t tkl = get_token_length();
	uint8_t complement = (tkl%4) == 0 ? 0 : 4 - (tkl%4); // TODO : check if we can use xor
	options_offset =  COAP_OFFSET_TOKEN + tkl + complement;
	return _eth->get_offset_payload(options_offset);
}

void Coap::get_mac_src(l2addr * mac_src) {
	_eth->get_mac_src(mac_src);
}
