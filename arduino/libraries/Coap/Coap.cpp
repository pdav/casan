#include "Coap.h"

Coap::Coap(uint8_t *mac_addr, uint8_t *eth_type)
{
	_eth = new EthernetRaw();
	_eth->setmac(mac_addr);
	_eth->setethtype(eth_type);
}

void Coap::emit_individual_coap(uint8_t *mac_addr_dest, 
		uint8_t type, 
		uint8_t code, 
		uint8_t *id,
		uint8_t token_length, 
		uint8_t *token, 
		uint8_t length,
		uint8_t *payload)
{
	uint8_t sbuf[BUFFER_SIZE] = {0};
	sbuf[COAP_OFFSET_TYPE] |= (type & 0x3) << 4;
	sbuf[COAP_OFFSET_TKL] |= (token_length & 0xF);
	sbuf[COAP_OFFSET_CODE] = code;
	memcpy(sbuf + COAP_OFFSET_ID, id, 2);
	memcpy(sbuf + COAP_OFFSET_TOKEN, token, token_length);

	{
		uint8_t payload_offset = 0;
		payload_offset = token_length + (4 - (token_length % 4));


		sbuf[COAP_OFFSET_TOKEN + payload_offset] = 0xFF;
		memcpy(sbuf + COAP_OFFSET_TOKEN + payload_offset + 1, payload, length);

		_eth->send(mac_addr_dest, sbuf, COAP_OFFSET_TOKEN + payload_offset + length +1);
	}

}
uint8_t Coap::decode_individual_coap(uint8_t *mac_addr_src, 
		uint8_t *type, 
		uint8_t *code, 
		uint8_t *id, 
		uint8_t *token_length,
		uint8_t *token, 
		uint8_t *length,
		uint8_t **payload)
{
	{
		int ret = _eth->recv(mac_addr_src, NULL, NULL);
		// if the packet received is not ok (not COAP / not for us / not the master)
		// return it
		if(ret != 0)
			return ret;
	}

	*type = ((*(_eth->get_offset_payload(COAP_OFFSET_TYPE))) >> 4 ) & 0x03;
	memcpy(code, _eth->get_offset_payload(COAP_OFFSET_CODE), 1);
	memcpy(id, _eth->get_offset_payload(COAP_OFFSET_ID), 2);

	*token_length = (*(_eth->get_offset_payload(COAP_OFFSET_TKL))) & 0x0F;
	if( *token_length > 0)
	{
		memcpy(token, _eth->get_offset_payload(COAP_OFFSET_TOKEN), *token_length);
	}
	else
	{
		*token = 0;
	}

	// we search for 0xF octet in the message, it's the payload start of the COAP message
	// we do that because there is no other option to know the option length
	{
		uint8_t payload_offset = 0;
		uint8_t tmp = 0;
		payload_offset = COAP_OFFSET_TOKEN + *token_length + (4 - (*token_length %4));

		tmp = *(_eth->get_offset_payload(payload_offset));
		while(tmp != 0xFF && payload_offset < _eth->get_payload_length())
		{
			payload_offset += 4;
			tmp = *(_eth->get_offset_payload(payload_offset)); 
		}
		// *length = _eth->get_payload_length() - (payload_offset + COAP_OFFSET_TOKEN + *token_length);

		++payload_offset;
	
		*length = 5; 	// TODO : FIXME
		*payload = _eth->get_offset_payload(payload_offset);
	}
	return 0;
}
uint8_t Coap::coap_available()
{
	return _eth->available(); // returns the payload
}

