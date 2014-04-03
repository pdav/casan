#ifndef __COAP_H__
#define __COAP_H__

#include "Arduino.h"
#include "defs.h"
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
	Coap (l2net *e) ;
	~Coap () ;

	void send (l2addr &mac_addr_dest, Message &m) ;
	l2_recv_t recv (Message &m) ;

	// TODO : tests
	// TODO : handle errors
	void encode (Message &m, uint8_t sbuf[], size_t &sbuflen) ;
	bool decode (Message &m, uint8_t rbuf[], size_t rbuflen) ;

	void get_mac_src (l2addr *mac_src) ;
	void set_master_addr (l2addr *master_addr) ;

    private:
	uint8_t get_type (void) ;
	uint8_t get_code (void) ;
	int get_id (void) ;
	uint8_t get_token_length (void) ;
	uint8_t *get_token (void) ;
	uint8_t get_payload_offset (void) ;

	l2net *l2_ ;
} ;

#endif
