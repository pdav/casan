#include "resource.h"


Resource::Resource(char *name, char *title, char *rt)
{
	_name = (char *) malloc(strlen(name)+1);
	memcpy(_name, name, strlen(name)+1);

	_title = (char *) malloc(strlen(title)+1);
	memcpy(_title, title, strlen(title)+1);

	_rt = (char *) malloc(strlen(rt)+1);
	memcpy(_rt, rt, strlen(rt)+1);
}

bool Resource::check_name(char *name, int len) 
{
	return strncmp(_name, name, len) == 0;
}

void Resource::add_handler(coap_code_t type, uint8_t (*handler)(Message &in, Message &out)) 
{
	switch(type) {
		case COAP_CODE_GET:
		     
			_thandler[0].handler = handler;
		break;
		case COAP_CODE_POST:
			_thandler[1].handler = handler;
		break;
		case COAP_CODE_PUT:
			_thandler[2].handler = handler;
		break;
		case COAP_CODE_DELETE:
			_thandler[3].handler = handler;
		break;
	}
	
}

handler_s Resource::get_handler(coap_code_t type) 
{
	switch(type) {
		case COAP_CODE_GET:
			return _thandler[0];
		break;
		case COAP_CODE_POST:
			return _thandler[1];
		break;
		case COAP_CODE_PUT:
			return _thandler[2];
		break;
		case COAP_CODE_DELETE:
			return _thandler[3];
		break;
	}
	
}
