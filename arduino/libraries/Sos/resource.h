#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "enum.h"
#include <Arduino.h>
#include "defs.h"
#include "message.h"

typedef struct handler_
{
    uint8_t (*handler) (Message &in, Message &out) ;
} handler_s;

class Resource {
    public:
	Resource (const char *name, const char *title, const char *rt) ;
	//~Resource();

	bool check_name (const char *name, int len);
	void add_handler (coap_code_t type, uint8_t (*handler) (Message &in, Message &out)) ;
	handler_s get_handler (coap_code_t type) ;

	// FIXME : make these private (they are used in rmanager.cpp)
	char *name_ ;
	char *title_ ;
	char *rt_ ;

    private :
	handler_s thandler_ [4];
};

#endif
