#include "message.h"

Message::Message() : _id(0), _token_length(0), _payload_length(0), _token(NULL), _payload(NULL) {
}

Message::Message( const Message & c) 
	: _type(c._type), _code(c._code), _token_length(c._token_length), 
	_payload_length(c._payload_length) {
		uint8_t *tmp_id = c._id;
		_id[0] = tmp_id[0];
		_id[1] = tmp_id[1];
		_token = c.get_token_copy();
		_payload = c.get_payload_copy();
}

Message & Message::operator=(const Message &m) {
	Message * ret = new Message();
	ret->set_id(m._id);
	ret->set_token(m._token_length, m._token);
	ret->set_payload(m._payload_length, m._payload);
	return *ret;
}

Message::~Message() {
	if(_token != NULL)
		free(_token);
	if(_payload != NULL)
		free(_payload);
}

uint8_t Message::get_type(void) {
	return _type;
}

uint8_t Message::get_code(void) {
	return _code;
}

uint8_t * Message::get_id(void) {
	return _id;
}

uint8_t Message::get_token_length(void) {
	return _token_length ;
}

uint8_t * Message::get_token(void){
	return _token ;
}

uint8_t * Message::get_token_copy(void) {
	uint8_t * copy = malloc(_token_length);
	memcpy(copy, _token, _token_length);
	return copy;
}

uint8_t Message::get_payload_length(void) {
	return _payload_length ;
}

uint8_t * Message::get_payload(void) {
	return _payload;
}

uint8_t * Message::get_payload_copy(void) {
	uint8_t * copy = malloc(_payload_length);
	memcpy(copy, _payload, _payload_length);
	return copy;
}

void Message::set_type(uint8_t t) {
	_type = t;
}

void Message::set_code(uint8_t c) {
	_code = c;
}

void Message::set_id(uint8_t *id) {
	memcpy(_id, id, 2);
}

void Message::set_token(uint8_t token_length, uint8_t *token){
	_token_length = token_length;
	_token = malloc(token_length);
	memcpy(_token, token, token_length);
}

void Message::set_payload(uint8_t payload_length, uint8_t * payload) {
	_payload_length = payload_length;
	_payload = malloc(payload_length);
	memcpy(_payload, payload, payload_length);
}

