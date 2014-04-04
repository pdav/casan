#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include "Arduino.h"
#include "defs.h"
#include "enum.h"
#include "option.h"
#include "memory_free.h"


class Message {
    public:
	Message () ;
	Message (Message &c) ;
	~Message () ;

	Message & operator= (const Message &m) ;
	bool operator== (const Message &m) ;

	// accessors (for received messages)
	uint8_t get_type (void) ;
	uint8_t get_code (void) ;
	int get_id (void) ;
	uint8_t get_toklen (void) ;
	uint8_t *get_token (void) ;
	uint8_t *get_token_copy (void) ;
	uint8_t get_paylen (void) ;
	uint8_t *get_payload (void) ;
	uint8_t *get_payload_copy (void) ;

	// give an option, and delete the entry in _opt_list
	option *pop_option (void) ;
	// give options one by one but without delete the entry in _opt_list
	// only use case : foreach option
	option *get_option (void) ;
	void reset_get_option (void) ;
	void reset (void) ;

	// mutators (to send messages)
	void set_type (uint8_t t) ;
	void set_code (uint8_t c) ;
	void set_id (int id) ;
	void set_token (uint8_t token_length, uint8_t *token) ;
	void set_payload (uint8_t payload_length, uint8_t *payload) ;

	// don't forget to delete the copy after sending (free_options)
	// this function adds options in the right order
	void push_option (option &o) ;

	void free_options (void) ;

	void print (void) ;

    private:
	struct optlist
	{
	    option *o ;
	    optlist *next ;
	} ;

	uint8_t type_ ;
	uint8_t code_ ;
	int id_ ;
	uint8_t toklen_ ;
	uint8_t *token_ ;
	uint8_t paylen_ ;
	uint8_t *payload_ ;
	uint8_t optlen_ ;
	optlist *optlist_ ;		// sorted list of all options
	optlist *curopt_ ;		// current option (position in opt list)
	bool curopt_initialized_ ;	// is curopt_ initialized ?
} ;

#endif
