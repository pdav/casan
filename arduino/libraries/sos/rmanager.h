#ifndef __RMANAGER_H__
#define __RMANAGER_H__

#include "sos.h"

// The resources manager

class Rmanager {

public:

	Rmanager();
	~Rmanager();

	void add_resource(char *name, uint8_t (*handler)(Message &in, Message &out));
	uint8_t request_resource(Message &in, Message &out); // TODO
	void delete_resource(rmanager_s *r);
	void reset();

private :

	rmanager_s * get_resource_instance(Message &in);
	uint8_t exec_request(rmanager_s *m, Message &in, Message &out);

	typedef struct {
		uint8_t (*h)(Message &in, Message &out);
		char *name;
		rmanager_s *s;
	} rmanager_s;
	rmanager_s *_resources = NULL;
}

#endif

