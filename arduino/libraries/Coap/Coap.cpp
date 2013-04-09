#include "Coap.h"

Coap::Coap(uint8_t *mac_addr, uint8_t *eth_type)
{
	_eth = new EthernetRaw();
	_eth->setmac(mac_addr);
	_eth->setethtype(eth_type);
}

void Coap::emit_individual_coap(uint8_t *mac_addr_dest, 
		int type, 
		uint8_t code, 
		uint8_t id,
		uint8_t token, 
		uint8_t *payload, 
		uint8_t length)
{
}
uint8_t Coap::decode_individual_coap(uint8_t *mac_addr_src, 
		int *type, 
		uint8_t *code, 
		int *id, 
		uint8_t *token_length,
		uint8_t *token, 
		uint8_t *payload, 
		uint8_t *length)
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
		uint8_t tmp_count = 0;
		uint8_t tmp = 0;
		while(tmp != 0xFF && tmp_count + COAP_OFFSET_TOKEN + *token_length < _eth->get_payload_length())
		{
			tmp = *(_eth->get_offset_payload(tmp_count + COAP_OFFSET_TOKEN + *token_length));
			tmp_count++;
		}
		*length = _eth->get_payload_length() - (tmp_count + COAP_OFFSET_TOKEN + *token_length);
		memcpy(payload, _eth->get_offset_payload(tmp_count + COAP_OFFSET_TOKEN + *token_length),  *length);
	}
	return 0;
}
uint8_t Coap::coap_available()
{
	return _eth->available(); // returns the payload
}

