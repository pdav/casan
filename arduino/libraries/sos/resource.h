#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "enum.h"
#include <Arduino.h>
#include "defs.h"
#include "message.h"

typedef struct handler_ {
	uint8_t (*handler)(Message &in, Message &out);
} handler_s;

class Resource {

public:

	Resource(char *name, char *title, char *rt);
	//~Resource();

	bool check_name(char *name, int len);
	void add_handler(coap_code_t type, uint8_t (*handler)(Message &in, Message &out));
	handler_s get_handler(coap_code_t type);

	char *_name;
	char *_title;
	char *_rt;
	

private :


	handler_s _thandler[4];
};

#endif
