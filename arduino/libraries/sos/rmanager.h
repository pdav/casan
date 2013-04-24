#ifndef __RMANAGER_H__
#define __RMANAGER_H__

#include "sos.h"

// The resources manager

class Rmanager {

public:

	Rmanager();
	~Rmanager();

	void add_resource(char *name, uint8_t (*handler)(Message &in, Message &out));
	uint8_t request_resource(char *name, enum coap_request_method, uint8_t *option);
	void delete_resource(char *name);
	void delete_resource(rmanager_s *r);
	void reset();

private :

	rmanager_s * get_resource_instance(char *name);
	uint8_t exec_request(char *name, enum coap_request_method coap_req, uint8_t *option);

	typedef struct {
		uint8_t (*h)(uint8_t, uint8_t*);
		char *name;
		rmanager_s *s;
	} rmanager_s;
	rmanager_s *_resources = NULL;
}

#endif

