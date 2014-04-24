/**
 * @file resource.h
 * @brief Resource class interface
 */

#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "enum.h"
#include <Arduino.h>
#include "defs.h"
#include "msg.h"

/**
 * @class Resource
 *
 * @brief An object of class Resource represents a resource which
 *	is provided by the application and managed by the SOS engine.
 *
 * This class represents a resource. A resource has:
 * - some attributes: name, title, etc.
 * - a handler for each CoAP operation (GET, PUT, etc.)
 * - a textual representation for `/.well-known/sos` aggregation
 *
 * The handler prototype (see the `handler_t` typedef) is:
 *	`uint8_t handler (Msg &in, Msg &out)`
 * The message `out` is prepared by the SOS engine with some items
 * from the incoming message (ACK, id, token) before calling the handler.
 * Rest of message must be provided by the handler, except code which
 * will be filled with the return value of the handler.
 * Note that the handler may provide the content-format option if
 * `text_plain` is not the wanted default.
 */

class Resource {
    public:
	Resource (const char *name, const char *title, const char *rt) ;
	~Resource() ;

	/** Handler prototype. See class description for details.
	 */
	typedef uint8_t (*handler_t) (Msg &in, Msg &out) ;

	/** Accessor function
	 *
	 * @return name of resource (do not free this string)
	 */
	char *name (void)	{ return name_ ; }

	void handler (coap_code_t op, handler_t h) ;
	handler_t handler (coap_code_t op) ;

	int well_known (char *buf, size_t maxlen) ;
	void print (void) ;

    private :
	handler_t handler_ [5] ;		// indexed by coap_code_t

	char *name_ ;
	char *title_ ;
	char *rt_ ;

};

#endif
