#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include "Arduino.h"
#include "util.h"

class Message {
	public:
		Message();
		Message(Message &c);
		~Message();

		Message & operator=(const Message &m);

		uint8_t get_type(void);
		uint8_t get_code(void);
		int get_id(void);
		char * get_name_copy(void);
		uint8_t get_token_length(void);
		uint8_t * get_token(void);
		uint8_t * get_token_copy(void);
		uint8_t get_payload_length(void);
		uint8_t * get_payload(void);
		uint8_t * get_payload_copy(void);
		uint8_t get_options_length(void);
		uint8_t * get_options(void);
		uint8_t * get_options_copy(void);

		void set_type(uint8_t t);
		void set_code(uint8_t c);
		void set_id(int id);
		void set_token(uint8_t token_length, uint8_t *token);
		void set_payload(uint8_t payload_length, uint8_t * payload);
		void set_options(uint8_t options_length, uint8_t * options);

	private:
		uint8_t _type;
		uint8_t _code;
		int _id;
		uint8_t _token_length;
		uint8_t *_token = NULL;
		uint8_t _payload_length;
		uint8_t *_payload = NULL;
		uint8_t _options_length;
		uint8_t *_options = NULL;
};

#endif
