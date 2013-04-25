#ifndef __COAP_H__
#define __COAP_H__

#include "Arduino.h"
#include "ethernetraw.h"
#include "message.h"

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
// We need to create a MACRO
enum coap_return_code {
	COAP_RETURN_CODE_200 = (0x2 << 5),
	COAP_RETURN_CODE_400 = (0x4 << 5)
};


// the offset to get pieces of information in the MAC payload
#define		COAP_OFFSET_TYPE	0
#define		COAP_OFFSET_TKL		0
#define		COAP_OFFSET_CODE	1
#define		COAP_OFFSET_ID		2
#define		COAP_OFFSET_TOKEN	4

class Coap {
	public:

		Coap(l2addr *addr, uint8_t *eth_type);
		Coap(EthernetRaw *e);
		Coap();
		void set_l2(EthernetRaw *e);
		uint8_t coap_available();
		uint8_t fetch(uint8_t *mac_addr_src);
		void send(l2addr *mac_addr_dest, Message *m);
		void recv(l2addr *mac_addr_src, Message *m);

		uint8_t get_type(void);
		uint8_t get_code(void);
		uint8_t *get_id(void);
		uint8_t get_token_length(void);
		uint8_t * get_token(void);
		uint8_t get_payload_length(void);
		uint8_t * get_payload(void);
		void get_mac_src(uint8_t * mac_src);

	private:
		EthernetRaw *_eth;
};

#endif
