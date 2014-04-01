#ifndef __RMANAGER_H__
#define __RMANAGER_H__

#include "message.h"
#include "coap.h"
#include "option.h"
#include "resource.h"

#define SOS_RESOURCES_ALL		"resources"

typedef struct _rmanager_s {
     Resource *r;
	_rmanager_s *s;
} rmanager_s;

class Rmanager {

public:

	Rmanager();
	~Rmanager();

	void add_resource(Resource *resource);
	uint8_t request_resource(Message &in, Message &out);
	void get_all_resources(Message &out);
	void delete_resource(rmanager_s *r);
	void reset();

	void print(void);

private :

	rmanager_s * get_resource_instance(option *o);

	rmanager_s *_resources = NULL;
};

#endif

