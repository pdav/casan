#ifndef __RMANAGER_H__
#define __RMANAGER_H__

#include "message.h"
#include "coap.h"
#include "option.h"

typedef struct _rmanager_s {
	uint8_t (*h)(Message &in, Message &out);
	char *name;
	int namelen;
	_rmanager_s *s;
} rmanager_s;

class Rmanager {

public:

	Rmanager();
	~Rmanager();

	void add_resource(char *name, int namelen,
			uint8_t (*handler)(Message &in, Message &out));
	uint8_t request_resource(Message &in, Message &out); // TODO
	void get_all_resources(Message &out);
	void delete_resource(rmanager_s *r);
	void reset();

private :

	rmanager_s * get_resource_instance(Message &in);
	uint8_t exec_request(Message &in, Message &out);

	rmanager_s *_resources = NULL;
};

#endif

