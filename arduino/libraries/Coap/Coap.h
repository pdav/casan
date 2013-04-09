#ifndef COAP_H
#define COAP_H

#include "Arduino.h"
#include "EthernetRaw.h"

#define		COAP_TYPE_CON	0
#define		COAP_TYPE_NON	1
#define		COAP_TYPE_ACK	2
#define		COAP_TYPE_RST	3

// the offset to get pieces of information in the MAC payload
#define		COAP_OFFSET_TYPE	0
#define		COAP_OFFSET_TKL		0
#define		COAP_OFFSET_CODE	1
#define		COAP_OFFSET_ID		2
#define		COAP_OFFSET_TOKEN	4

class Coap {
	public:
		Coap(uint8_t *mac_addr, uint8_t *eth_type);
		void emit_individual_coap(uint8_t *mac_addr_dest, 
				int type, 
				uint8_t code, 
				uint8_t id,
			   	uint8_t token, 
				uint8_t *payload, 
				uint8_t length);
		uint8_t decode_individual_coap(uint8_t *mac_addr_src, 
				int *type, 
				uint8_t *code, 
				int *id, 
				uint8_t *token_length,
				uint8_t *token, 
				uint8_t *payload, 
				uint8_t *length);
		uint8_t coap_available();
	private:
		EthernetRaw *_eth;
};

#endif
