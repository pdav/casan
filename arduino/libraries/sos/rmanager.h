#ifndef __RMANAGER_H__
#define __RMANAGER_H__

#include "message.h"
#include "coap.h"
#include "option.h"

#define SOS_RESOURCES_ALL		"/resources"

typedef struct _rmanager_s {
	uint8_t (*h)(Message &in, Message &out);
	char *name;
	int namelen;
	char *title;
	int titlelen;
	char *rt;
	int rtlen;
	_rmanager_s *s;
} rmanager_s;

class Rmanager {

public:

	Rmanager();
	~Rmanager();

	void add_resource(char *name, int namelen,
			char *title, int titlelen,
			char *rt, int rtlen,
			uint8_t (*handler)(Message &in, Message &out));
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

