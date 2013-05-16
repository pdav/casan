#include "coap.h"

Coap::Coap(EthernetRaw *e) {
	_eth = e;
}

Coap::~Coap(void) {
	delete _eth;
}

void Coap::send(l2addr &mac_addr_dest, Message &m) {
	uint8_t sbuf[BUFFER_SIZE] = {0};
	sbuf[COAP_OFFSET_TYPE] |= (m.get_type() & 0x3) << 4;
	sbuf[COAP_OFFSET_TKL] |= (m.get_token_length() & 0xF);
	sbuf[COAP_OFFSET_CODE] = m.get_code();
	int id = m.get_id();
	memcpy(sbuf + COAP_OFFSET_ID, &id + 1, 1);
	memcpy(sbuf + COAP_OFFSET_ID +1, &id, 1);
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
	return _eth->get_payload_length() - 
		(get_payload_offset() + COAP_OFFSET_TOKEN + get_token_length());
}

uint8_t Coap::get_payload_offset(void) {
	uint8_t payload_offset = 0;
	uint8_t tmp = 0;
	uint8_t tkl = get_token_length();
	payload_offset = (tkl == 0 || tkl == 4) 
		? COAP_OFFSET_TOKEN + tkl : COAP_OFFSET_TOKEN + tkl + (4 - (tkl % 4));
	tmp = *(_eth->get_offset_payload(payload_offset));
	while(tmp != 0xFF && payload_offset < _eth->get_payload_length()) {
		payload_offset++;
		tmp = *(_eth->get_offset_payload(payload_offset)); 
	}
	++payload_offset;
	return payload_offset;
}

uint8_t * Coap::get_payload(void) {
	return _eth->get_offset_payload(get_payload_offset());
}

// FIXME : probably not working for now
uint8_t Coap::get_options_length(void) { 
	bool success;
	int i = get_token_length() + 4;
	int opt_nb = 0 ;
	uint8_t * msg_ = _eth->get_offset_payload(0);
	int msglen_ = _eth->get_payload_length();

	while (msg_[i] != 0xff && i < msglen_)
	{
		int opt_delta, opt_len ;

		opt_delta = (msg_ [i] >> 4) & 0x0f ;
		opt_len   = (msg_ [i]     ) & 0x0f ;
		i++ ;
		switch (opt_delta)
		{
			case 13 :
				opt_delta = msg_ [i] - 13 ;
				i += 1 ;
				break ;
			case 14 :
				opt_delta = (msg_ [i] << 8) + msg_ [i+1] - 269 ;
				i += 2 ;
				break ;
			case 15 :
				PRINT_DEBUG_STATIC("\033[31mreceive error : opt delta\033[00m")
				success = false;
				break ;
		}
		opt_nb += opt_delta ;

		switch (opt_len)
		{
			case 13 :
				opt_len = msg_ [i] - 13 ;
				i += 1 ;
				break ;
			case 14 :
				opt_len = (msg_ [i] << 8) + msg_ [i+1] - 269 ;
				i += 2 ;
				break ;
			case 15 :
				PRINT_DEBUG_STATIC("\033[31mreceive error : opt len\033[00m ")
				success = false;
				break ;
		}

		// get option value
		// XXXX
		PRINT_DEBUG_STATIC("OPTION ")
		PRINT_DEBUG_DYNAMIC(opt_nb)

		i += opt_len ;
	}

	if (msg_ [i] != 0xff)
	{
		PRINT_DEBUG_STATIC("\033[31mreceive error : msg[i] != 0xff")
	}
	i++ ;
	if(! success) {
		PRINT_DEBUG_STATIC( "\033[31m ! SUCCESS opt \033[00m");
		return 0;
	}
	else {
		return i - get_token_length() - 4; // option length
	}
}

uint8_t * Coap::get_options(void) { // TODO : is it the way it works ?
	uint8_t options_offset = 4 + get_token_length();
	return _eth->get_offset_payload(options_offset);
}

void Coap::get_mac_src(l2addr * mac_src) {
	_eth->get_mac_src(mac_src);
}
