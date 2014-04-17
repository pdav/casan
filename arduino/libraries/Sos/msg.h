#ifndef __MSG_H__
#define __MSG_H__

#include "Arduino.h"
#include "defs.h"
#include "enum.h"
#include "l2.h"
#include "option.h"

#define COAP_RETURN_CODE(x,y) ((x << 5) | (y & 0x1f))

// the offset to get pieces of information in the MAC payload
#define		COAP_OFFSET_TYPE	0
#define		COAP_OFFSET_TKL		0
#define		COAP_OFFSET_CODE	1
#define		COAP_OFFSET_ID		2
#define		COAP_OFFSET_TOKEN	4

class Msg {
    public:
	Msg () ;
	Msg (const Msg &c) ;
	~Msg () ;

	Msg &operator= (const Msg &m) ;
	bool operator== (const Msg &m) ;

	void reset (void) ;

	// Receive message
	l2_recv_t recv (l2net &l2) ;

	// accessors (for received messages)
	uint8_t  get_type (void)	{ return type_ ; }
	uint8_t  get_code (void)	{ return code_ ; }
	uint16_t get_id (void)		{ return id_ ; }
	uint8_t  get_toklen (void)	{ return toklen_ ; }
	uint8_t *get_token (void)	{ return token_ ; }
	uint16_t get_paylen (void)	{ return paylen_ ; }
	uint8_t *get_payload (void)	{ return payload_ ; }
	void     get_src (l2addr *mac) ;

	uint8_t *get_payload_copy (void) ;

	// give an option, and delete the entry in _opt_list
	option *pop_option (void) ;
	// give options one by one but without delete the entry in _opt_list
	// only use case : foreach option
	option *next_option (void) ;
	void reset_next_option (void) ;

	// Send message

	bool send (l2net &l2, l2addr &dest) ;
	// mutators (to send messages)
	void set_type (uint8_t t)	{ type_ = t ; }
	void set_code (uint8_t c)	{ code_ = c ; }
	void set_id   (uint16_t id)	{ id_ = id ; }

	void set_token (uint8_t toklen, uint8_t *token) ;
	void set_payload (uint8_t *payload, uint16_t paylen) ;

	void push_option (option &o) ;

	// not so basic methods
	void content_format (bool reset, option::content_format cf) ;
	option::content_format content_format (void) ;

	void print (void) ;

    private:
	struct optlist
	{
	    option *o ;
	    optlist *next ;
	} ;

	// TODO : tests, handle errors
	bool coap_encode (uint8_t sbuf [], size_t &sbuflen) ;
	bool coap_decode (uint8_t rbuf [], size_t rbuflen, bool truncated) ;

	void msgcopy (const Msg &m) ;

	l2net   *l2_ ;
	uint8_t *encoded_ ;	// encoded message to send
	uint16_t enclen_ ;	// real size of msg (encoded_ may be larger)

	uint8_t  type_ ;
	uint8_t  code_ ;
	uint16_t id_ ;
	uint8_t  toklen_ ;
	uint8_t  token_ [COAP_MAX_TOKLEN] ;
	uint16_t paylen_ ;
	uint8_t *payload_ ;
	uint8_t  optlen_ ;
	optlist *optlist_ ;		// sorted list of all options
	optlist *curopt_ ;		// current option (position in opt list)
	bool     curopt_initialized_ ;	// is curopt_ initialized ?
} ;

#endif
