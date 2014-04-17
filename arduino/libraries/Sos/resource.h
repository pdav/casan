#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "enum.h"
#include <Arduino.h>
#include "defs.h"
#include "msg.h"

typedef struct handler_
{
    uint8_t (*handler) (Msg &in, Msg &out) ;
} handler_s;

class Resource {
    public:
	Resource (const char *name, const char *title, const char *rt) ;
	~Resource() ;

	bool check_name (const char *name);
	void add_handler (coap_code_t type, uint8_t (*handler) (Msg &in, Msg &out)) ;

	int get_well_known (char *buf, size_t maxlen) ;
	void print (void) ;

	handler_s get_handler (coap_code_t type) ;

    private :
	handler_s thandler_ [4];

	char *name_ ;
	char *title_ ;
	char *rt_ ;

};

#endif
