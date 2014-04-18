#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "enum.h"
#include <Arduino.h>
#include "defs.h"
#include "msg.h"

typedef uint8_t (*handler_t) (Msg &in, Msg &out) ;

class Resource {
    public:
	Resource (const char *name, const char *title, const char *rt) ;
	~Resource() ;

	bool check_name (const char *name);
	void handler (coap_code_t op, handler_t h) ;
	handler_t handler (coap_code_t op) ;

	int get_well_known (char *buf, size_t maxlen) ;
	void print (void) ;

    private :
	// handler_s thandler_ [4];
	handler_t handler_ [4] ;

	char *name_ ;
	char *title_ ;
	char *rt_ ;

};

#endif
