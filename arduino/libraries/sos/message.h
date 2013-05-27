#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include "Arduino.h"
#include "util.h"
#include "defs.h"
#include "enum.h"
#include "option.h"
#include "memory_free.h"


class Message {
	public:
		Message();
		Message(Message &c);
		~Message();

		Message & operator=(const Message &m);

		// accessors (for received messages)
		uint8_t get_type(void);
		uint8_t get_code(void);
		int get_id(void);
		uint8_t get_token_length(void);
		uint8_t * get_token(void);
		uint8_t * get_token_copy(void);
		uint8_t get_payload_length(void);
		uint8_t * get_payload(void);
		uint8_t * get_payload_copy(void);

		// TODO : check
		// give an option, and delete the entry in _opt_list
		option * pop_option (void) ;
		// give options one by one but without delete the entry in _opt_list
		// only use case : foreach option
		option * get_option (void) ;

		// mutators (to send messages)
		void set_type(uint8_t t);
		void set_code(uint8_t c);
		void set_id(int id);
		void set_token(uint8_t token_length, uint8_t *token);
		void set_payload(uint8_t payload_length, uint8_t *payload);
		// TODO : check
		// I copy the option
		// we have to think to delete the copy after sending (free_options)
		// this function needs to add option in the right order
		void push_option(option &o);

		// TODO : check
		void free_options(void);

		void print(void);

	private:

		typedef struct _opt_list_s {
			option *o;
			_opt_list_s *s;	// next option
		} opt_list_s;

		uint8_t _type = 0;
		uint8_t _code = 0;
		int _id = 0;
		uint8_t _token_length = 0;
		uint8_t *_token = NULL;
		uint8_t _payload_length = 0;
		uint8_t *_payload = NULL;
		uint8_t _options_length = 0;
		uint8_t *_options = NULL;
		opt_list_s * _opt_list = NULL;	// the option's list
};

#endif
