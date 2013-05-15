#ifndef __COAP_H__
#define __COAP_H__

#include "Arduino.h"
#include "defs.h"
#include "ethernetraw.h"
#include "l2.h"
#include "message.h"


#define COAP_RETURN_CODE(x,y) ((x << 5) | (y & 0x1f))

// the offset to get pieces of information in the MAC payload
#define		COAP_OFFSET_TYPE	0
#define		COAP_OFFSET_TKL		0
#define		COAP_OFFSET_CODE	1
#define		COAP_OFFSET_ID		2
#define		COAP_OFFSET_TOKEN	4

class Coap {
	public:

		Coap(EthernetRaw *e);
		~Coap();
		uint8_t coap_available();
		uint8_t fetch(void);
		void send(l2addr &mac_addr_dest, Message &m);
		uint8_t recv(Message &m);

		uint8_t get_type(void);
		uint8_t get_code(void);
		int get_id(void);
		uint8_t get_token_length(void);
		uint8_t * get_token(void);

		// TODO / FIXME
		uint8_t get_payload_length(void);
		uint8_t * get_payload(void);
		uint8_t get_options_length(void);
		uint8_t * get_options(void);

		void get_mac_src(l2addr * mac_src);

	private:
		uint8_t get_payload_offset(void);
		EthernetRaw *_eth;
};

#endif
