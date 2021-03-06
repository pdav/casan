/**
 * @file msg.h
 * @brief Msg class interface
 */

#ifndef __MSG_H__
#define __MSG_H__

#include "l2.h"
#include "option.h"
#include "token.h"
#include "time.h"

#define COAP_RETURN_CODE(x,y) ((x << 5) | (y & 0x1f))

// the offset to get pieces of information in the MAC payload
#define	COAP_OFFSET_TYPE	0
#define	COAP_OFFSET_TKL		0
#define	COAP_OFFSET_CODE	1
#define	COAP_OFFSET_ID		2
#define	COAP_OFFSET_TOKEN	4

/** CoAP methods */
typedef enum coap_code {
    COAP_CODE_EMPTY = 0,
    COAP_CODE_GET,
    COAP_CODE_POST,
    COAP_CODE_PUT,
    COAP_CODE_DELETE
} coap_code_t;

/** CoAP message types */
typedef enum coap_type {
    COAP_TYPE_CON = 0,
    COAP_TYPE_NON,
    COAP_TYPE_ACK,
    COAP_TYPE_RST
} coap_type_t;

/**
 * @brief An object of class Msg represents a message,
 * either received or to be sent
 *
 * This class represents messages to be sent to the network, or
 * received from the network.
 *
 * Message attributes are tied to CoAP specification: a message has
 * a type (CON, NON, ACK, RST), a code (GET, POST, PUT, DELETE, or a numeric
 * value for an answer in an ACK), an Id, a token, some options and
 * a payload.
 *
 * In order to be sent to the network, a message is transparently
 * encoded (by the `send` method) according to CoAP specification.
 * Similarly, a message is transparently decoded (by the `recv`
 * method) upon reception according to the CoAP specification.
 *
 * A received message is stored in the appropriate L2 receive buffer
 * (see the various L2net-* classes). This buffer is allocated at
 * the program startup and never freed. As such, there can be at most
 * one received message.
 */

class Msg {
    public:
	Msg (l2net *l2) ;
	Msg (const Msg &c) ;
	~Msg () ;

	Msg &operator= (const Msg &m) ;
	bool operator== (const Msg &m) ;

	void reset (void) ;

	// Receive message
	l2net::l2_recv_t recv (void) ;

	// accessors (for received messages)
	uint8_t  get_type    (void)	{ return type_ ; }
	uint8_t  get_code    (void)	{ return code_ ; }
	uint16_t get_id      (void)	{ return id_ ; }
	Token   &get_token   (void)	{ return token_ ; }
	uint16_t get_paylen  (void)	{ return paylen_ ; }
	uint8_t *get_payload (void)	{ return payload_ ; }

	// Send message

	bool send (l2addr &dest) ;
	// mutators (to send messages)
	void set_type    (uint8_t t)	{ type_ = t ; }
	void set_code    (uint8_t c)	{ code_ = c ; }
	void set_id      (uint16_t id)	{ id_ = id ; }
	void set_token   (Token &tok)	{ token_ = tok ; }
	void set_payload (uint8_t *payload, uint16_t paylen) ;

	// Option management

	// give an option, and delete the entry in _opt_list
	option *pop_option (void) ;	// to analyze a received message
	// give options one by one but without delete the entry in _opt_list
	// only use case : foreach option
	option *next_option (void) ;	// loop through options
	void reset_next_option (void) ;	// reset loop
	void push_option (option &o) ;	// to prepare to send a message
	option *search_option (option::optcode_t c) ;

	// not so basic methods
	size_t avail_space (void) ;	// available space in msg
	void content_format (bool reset, option::content_format cf) ;
	option::content_format content_format (void) ;

	void max_age (bool reset, time_t duration) ;
	time_t max_age (void) ;

	// debug method
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
	size_t coap_size (bool emulpayload) ;	// size of coap-encoded message

	void msgcopy (const Msg &m) ;

	l2net   *l2_ ;
	uint8_t *encoded_ ;	// encoded message to send
	uint16_t enclen_ ;	// real size of msg (encoded_ may be larger)

	uint8_t  type_ ;
	uint8_t  code_ ;
	uint16_t id_ ;
	Token    token_ ;
	uint16_t paylen_ ;
	uint8_t *payload_ ;
	uint8_t  optlen_ ;
	optlist *optlist_ ;		// sorted list of all options
	optlist *curopt_ ;		// current option (position in opt list)
	bool     curopt_initialized_ ;	// is curopt_ initialized ?
} ;

#endif
