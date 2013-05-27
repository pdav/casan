#include "message.h"

Message::Message() : _id(0), _token_length(0), 
	_payload_length(0), _options_length(0) {
	_payload = NULL;
	_options = NULL;
}

Message::Message( Message & c) 
	: _id(c._id),  _type(c._type), _code(c._code), 
	_token_length(c._token_length), 
	_payload_length(c._payload_length) {
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
		delete _token;
	if(_payload != NULL)
		delete _payload;
	free_options();
}

uint8_t Message::get_type(void) {
	return _type;
}

uint8_t Message::get_code(void) {
	return _code;
}

int Message::get_id(void) {
	return _id;
}

uint8_t Message::get_token_length(void) {
	return _token_length ;
}

uint8_t * Message::get_token(void){
	return _token ;
}

uint8_t * Message::get_token_copy(void) {
	uint8_t * copy = (uint8_t*) malloc(_token_length);
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
	uint8_t * copy = (uint8_t*) malloc(_payload_length);
	memcpy(copy, _payload, _payload_length);
	return copy;
}

// TODO : check
option * Message::pop_option(void) {
	option * res;
	if(NULL == _opt_list) {
		res = NULL;
	}
	else {
		res = _opt_list->o;
		opt_list_s * tmp = _opt_list->s;
		delete _opt_list;
		_opt_list = tmp;
	}
	return res;
}

void Message::set_type(uint8_t t) {
	_type = t;
}

void Message::set_code(uint8_t c) {
	_code = c;
}

void Message::set_id(int id) {
	_id = id;
}

void Message::set_token(uint8_t token_length, uint8_t *token){
	_token_length = token_length;
	_token = (uint8_t*) malloc(token_length);
	memcpy(_token, token, token_length);
}

void Message::set_payload(uint8_t payload_length, uint8_t * payload) {
	_payload_length = payload_length;
	_payload = (uint8_t*) malloc(payload_length);
	memcpy(_payload, payload, payload_length);
}

// TODO : check
void Message::push_option(option &o) {
	opt_list_s * tmp = (opt_list_s*) malloc(sizeof(opt_list_s));
	opt_list_s * tmp2 = _opt_list;
	tmp->o = new option(o);
	if(_opt_list == NULL) {
		PRINT_DEBUG_STATIC("\033[32mpush_option : _opt_list == NULL\033[00m");
		tmp->s = NULL;
		_opt_list = tmp;
	} else if(*(tmp->o) < *(_opt_list->o)) {
		PRINT_DEBUG_STATIC("\033[32mpush_option : tmp->o < tmp2->o\033[00m");
		tmp->s = _opt_list;
		_opt_list = tmp;
	} else {
		opt_list_s * tmp3 = tmp2;
		while(!(*(tmp->o) < *(tmp2->o)) && tmp2->s != NULL) {
			PRINT_DEBUG_STATIC("boucle");
			tmp3 = tmp2;
			tmp2 = tmp2->s;
		}
		if(*(tmp->o) < *(tmp2->o)) {
			PRINT_DEBUG_STATIC("\033[32mpush_option :\033[00m *(tmp->o) < *(tmp2->o)");
			tmp3->s = tmp;
			tmp->s = tmp2;
		} else if(tmp2->s == NULL) {
			PRINT_DEBUG_STATIC("\033[32mpush_option : \033[00mtmp2->s == NULL");
			tmp2->s = tmp;
			tmp->s = NULL;
		} else {
			PRINT_DEBUG_STATIC("\033[31merror push_option\033[00m");
		}
	}
}

// TODO : check
void Message::free_options(void) {
	while(_opt_list != NULL) {
		PRINT_DEBUG_STATIC("free_options");
		delete pop_option();
	}
}

// TODO : check
option * Message::get_option(void) {
	static opt_list_s * ptr = _opt_list;
	if(ptr == NULL) {
		ptr = _opt_list;
		return NULL;
	}
	opt_list_s * tmp = ptr;
	ptr = ptr->s;
	return tmp->o;
}

void Message::print(void) {
	PRINT_FREE_MEM;
	{
		Serial.print(F("msg\tid = "));
		Serial.println(get_id());
		Serial.print(F("msg\ttype = "));
		Serial.println(get_type());
		/*
		   Serial.print(F("msg\tversion = "));
		   Serial.println(get_version());
		   */
		Serial.print(F("msg\ttoken_length = "));
		Serial.println(get_token_length());
		Serial.print(F("msg\ttoken = "));
		uint8_t * token = get_token();
		for(int i(0) ; i < get_token_length() ; i++)
			Serial.print(token[i], HEX);
		Serial.println();
	}

	{
		Serial.print(F("msg\tpayload_length = "));
		Serial.println(get_payload_length());
		Serial.print(F("msg\tpayload = "));
		uint8_t * payload = get_payload();
		for(int i(0) ; i < get_payload_length() ; i++)
			Serial.print(payload[i], HEX);
		Serial.println();
	}

	{

		option *o = NULL;
		for(o = get_option() ; o != NULL ; o = get_option()) {
			Serial.println(F("msg option\toptcode : "));
		}
	}

}
