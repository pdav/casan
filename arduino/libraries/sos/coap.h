#ifndef __COAP_H__
#define __COAP_H__

#include "Arduino.h"
#include "defs.h"
#include "ethernetraw.h"
#include "l2.h"
#include "option.h"
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

		void send(l2addr &mac_addr_dest, Message &m);
		eth_recv_t recv(Message &m);

		// TODO : tests
		// TODO : handle errors
		bool encode (Message &m, uint8_t sbuf[], size_t &sbuflen);
		bool decode (Message &m, uint8_t rbuf[], size_t rbuflen);

		void get_mac_src(l2addr * mac_src);

	private:

		uint8_t get_type(void);
		uint8_t get_code(void);
		int get_id(void);
		uint8_t get_token_length(void);
		uint8_t * get_token(void);
		uint8_t get_payload_offset(void);
		EthernetRaw *_eth;
};

#endif
