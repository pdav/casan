#ifndef COAP_H
#define COAP_H

#include "Arduino.h"
#include "EthernetRaw.h"

enum coap_request_method {
	COAP_CODE_GET,
	COAP_CODE_PUT,
	COAP_CODE_POST,
	COAP_CODE_DELETE
} ;

enum coap_message_type {
	COAP_MES_TYPE_CON,
	COAP_MES_TYPE_NON,
	COAP_MES_TYPE_ACK,
	COAP_MES_TYPE_RST
} ;

// TODO : it's just an example for now
enum coap_return_code {
	COAP_RETURN_CODE_200 = (0x2 << 5),
	COAP_RETURN_CODE_400 = (0x4 << 5
};


// the offset to get pieces of information in the MAC payload
#define		COAP_OFFSET_TYPE	0
#define		COAP_OFFSET_TKL		0
#define		COAP_OFFSET_CODE	1
#define		COAP_OFFSET_ID		2
#define		COAP_OFFSET_TOKEN	4

class Coap {
	public:

		Coap(uint8_t *mac_addr, uint8_t *eth_type);
		Coap(EthernetRaw *e);
		Coap();
		void set_l2(EthernetRaw *e);
		void emit_individual_coap(uint8_t *mac_addr_dest, 
				uint8_t type, 
				uint8_t code, 
				uint8_t *id,
				uint8_t token_length,
			   	uint8_t *token, 
				uint8_t length,
				uint8_t *payload);
		uint8_t decode_individual_coap(
				uint8_t *type, 
				uint8_t *code, 
				uint8_t *id, 
				uint8_t *token_length,
				uint8_t *token, 
				uint8_t *payload_length,
				uint8_t **payload);

		uint8_t coap_available();
		uint8_t fetch(uint8_t *mac_addr_src);
		void send(uint8_t *mac_addr_dest);

		uint8_t get_type(void);
		uint8_t get_code(void);
		uint8_t get_id(void);
		uint8_t get_token_length(void);
		uint8_t *get_token(void);
		uint8_t get_payload_length(void);
		uint8_t * get_payload(void);

		void set_type(uint8_t t);
		void set_code(uint8_t c);
		void set_id(uint8_t id);
		void set_token_length(uint8_t tkl);
		void set_token(uint8_t *token);
		void set_payload_length(uint8_t pll);
		void set_payload(uint8_t * payload);
	private:
		EthernetRaw *_eth;
		
		uint8_t _type;
		uint8_t _code;
		uint8_t _id;
		uint8_t _token_length;
		uint8_t *_token;
		uint8_t _payload_length;
		uint8_t *_payload;
};

#endif
