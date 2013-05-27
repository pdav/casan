#include "coap.h"

Coap::Coap(EthernetRaw *e) {
	_eth = e;
}

Coap::~Coap(void) {
	delete _eth;
}

void Coap::send(l2addr &mac_addr_dest, Message &m) {
	uint8_t sbuf[BUFFER_SIZE] = {0};
	size_t sbuflen(0);
	encode(m, sbuf, sbuflen);
	_eth->send( mac_addr_dest, sbuf, sbuflen);
}

// mac_addr of the master or broadcast, the message we get
uint8_t Coap::recv(Message &m) {
	uint8_t ret = fetch();
	if(ret != 0)
		return ret;
	decode(m, _eth->get_offset_payload(0), (size_t) _eth->get_payload_length());
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

void Coap::get_mac_src(l2addr * mac_src) {
	_eth->get_mac_src(mac_src);
}

/* returns true if message is decoding was successful */
bool Coap::decode(Message &m, uint8_t rbuf[], size_t rbuflen) {
	m.set_type(get_type());
	m.set_code(get_code());
	m.set_id(get_id());
	m.set_token(get_token_length(), get_token());

	bool success ;

	int i ;
	int opt_nb ;
	rbuflen = _eth->get_payload_length();
	int paylen_;

	success = true ;
	i = 6 + get_token_length();

	/*
	 * Options analysis
	 */

	opt_nb = 0 ;
	while (success && i < rbuflen && rbuf [i] != 0xff)
	{
		int opt_delta, opt_len ;
		option o ;

		opt_delta = (rbuf [i] >> 4) & 0x0f ;
		opt_len   = (rbuf [i]     ) & 0x0f ;
		i++ ;
		switch (opt_delta)
		{
			case 13 :
				opt_delta = rbuf [i] - 13 ;
				i += 1 ;
				break ;
			case 14 : // the opt_delta is on two bytes (the next)
				opt_delta = (rbuf [i] << 8) + rbuf [i+1] - 269 ;
				i += 2 ;
				break ;
			case 15 :
				success = false ;			// recv failed
				break ;
		}
		opt_nb += opt_delta ;

		switch (opt_len)
		{
			case 13 :
				opt_len = rbuf [i] - 13 ;
				i += 1 ;
				break ;
			case 14 : // the length is encoded on two bytes (after opt_delta)
				opt_len = (rbuf [i] << 8) + rbuf [i+1] - 269 ;
				i += 2 ;
				break ;
			case 15 :
				success = false ;			// recv failed
				break ;
		}

		/* register option */
		if (success)
		{
			Serial.print(F("OPTION opt=")); 
			Serial.print(opt_nb); 
			Serial.print(F(", len="));
			Serial.println(opt_len);
			o.optcode (option::optcode_t (opt_nb)) ;
			o.optval ((void *)(rbuf + i), opt_len) ;
			m.push_option (o) ;

			i += opt_len ;
		}
		else
			PRINT_DEBUG_STATIC ("OPTION unrecognized") ;

		paylen_ = rbuflen - i - 1 ;
		if (success && paylen_ > 0)
		{
			if (rbuf [i] != 0xff)
			{
				success = false ;
			}
			else
			{
				i++ ;
				m.set_payload(paylen_, rbuf + i);
			}
		}
		else paylen_ = 0 ;			// protect further operations
	}

	return success ;
}

bool Coap::encode (Message &m, uint8_t sbuf[], size_t &sbuflen) {
	bool success;
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
		/*
		sbuf[COAP_OFFSET_TOKEN + payload_offset] = 0xFF;
		memcpy(sbuf + COAP_OFFSET_TOKEN + payload_offset + 1, 
				m.get_payload(), (unsigned int) m.get_payload_length());
				*/
	}

	int i ;
	int opt_nb ;

	/*
	 * Format message, part 1 : compute message size
	 */

	i = 6 + m.get_token_length();

	opt_nb = 0 ;
	for(option * o = m.get_option() ; o != NULL ; o = m.get_option()) {
		int opt_delta, opt_len ;

		sbuflen++ ;			// 1 byte for opt delta & len

		opt_delta = o->optcode_ - opt_nb ;
		if (opt_delta >= 269)		// delta >= 269 => 2 bytes
			sbuflen += 2 ;
		else if (opt_delta >= 13)	// delta \in [13..268] => 1 byte
			sbuflen += 1 ;
		opt_nb = o->optcode_ ;

		opt_len = o->optlen_ ;
		if (opt_len >= 269)		// len >= 269 => 2 bytes
			sbuflen += 2 ;
		else if (opt_len >= 13)		// len \in [13..268] => 1 byte
			sbuflen += 1 ;
		sbuflen += o->optlen_ ;
	}
	if (sbuflen > 0)
		sbuflen += 1 + sbuflen ;	// don't forget 0xff byte

	/*
	 * Format message, part 2 : build message
	 */

	i = 0 ;

	// header
	sbuf [i++] = FORMAT_BYTE0 (SOS_VERSION, m.get_type(), m.get_token_length()) ;
	sbuf [i++] = m.get_code() ;
	sbuf [i++] = BYTE_HIGH (m.get_id()) ;
	sbuf [i++] = BYTE_LOW  (m.get_id()) ;
	// token
	if (m.get_token_length() > 0)
	{
		memcpy (sbuf + i, m.get_token(), m.get_token_length()) ;
		i += m.get_token_length() ;
	}
	// options
	opt_nb = 0 ;
	for(option * o = m.get_option() ; o != NULL ; o = m.get_option()) {
		int opt_delta, opt_len ;
		int posoptheader = i ;

		sbuf [posoptheader] = 0 ;

		i++ ;
		opt_delta = int (o->optcode_) - opt_nb ;
		if (opt_delta >= 269)		// delta >= 269 => 2 bytes
		{
			opt_delta -= 269 ;
			sbuf [i++] = BYTE_HIGH (opt_delta) ;
			sbuf [i++] = BYTE_LOW  (opt_delta) ;
			sbuf [posoptheader] |= 0xe0 ;
		}
		else if (opt_delta >= 13)	// delta \in [13..268] => 1 byte
		{
			opt_delta -= 13 ;
			sbuf [i++] = BYTE_LOW (opt_delta) ;
			sbuf [posoptheader] |= 0xd0 ;
		}
		else
		{
			sbuf [posoptheader] |= (opt_delta << 4) ;
		}
		opt_nb = o->optcode_ ;

		opt_len = o->optlen_ ;
		if (opt_len >= 269)		// len >= 269 => 2 bytes
		{
			opt_len -= 269 ;
			sbuf [i++] = BYTE_HIGH (opt_len) ;
			sbuf [i++] = BYTE_LOW  (opt_len) ;
			sbuf [posoptheader] |= 0x0e ;
		}
		else if (opt_len >= 13)		// len \in [13..268] => 1 byte
		{
			sbuf [i++] = BYTE_LOW (opt_len) ;
			sbuf [posoptheader] |= 0x0d ;
		}
		else
		{
			sbuf [posoptheader] |= opt_len ;
		}
		memcpy (sbuf + i, OPTVAL (o), o->optlen_) ;
		i += o->optlen_ ;
	}
	// payload
	if (sbuflen > 0)
	{
		sbuf [i++] = 0xff ;			// start of payload
		memcpy (sbuf + i, m.get_payload(), sbuflen) ;
	}

	success = true;
	return success;
}
