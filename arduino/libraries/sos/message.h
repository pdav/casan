#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include "sos.h"

class Message {
	public:
		Message();
		~Message();

		Message & operator=(const Message &m);

		uint8_t get_type(void);
		uint8_t get_code(void);
		uint8_t * get_id(void);
		uint8_t get_token_length(void);
		uint8_t * get_token(void);
		uint8_t * get_token_copy(void);
		uint8_t get_payload_length(void);
		uint8_t * get_payload(void);
	 	uint8_t * get_payload_copy(void);

		void set_type(uint8_t t);
		void set_code(uint8_t c);
		void set_id(uint8_t *id);
		void set_token(uint8_t token_length, uint8_t *token);
		void set_payload(uint8_t payload_length, uint8_t * payload);

	private:
		uint8_t _type;
		uint8_t _code;
		uint8_t _id[2];
		uint8_t _token_length;
		uint8_t *_token = NULL;
		uint8_t _payload_length;
		uint8_t *_payload = NULL;
}
#endif

