#ifndef __RMANAGER_H__
#define __RMANAGER_H__

#include "message.h"
#include "coap.h"
#include "option.h"
#include "resource.h"

#define SOS_RESOURCES_ALL		"resources"

struct rmanager_list
{
    Resource *r ;
    rmanager_list *next ;
} ;

class Rmanager {
    public:
	Rmanager () ;
	~Rmanager () ;

	void add_resource (Resource *resource) ;
	uint8_t request_resource (Message &in, Message &out) ;
	void get_all_resources (Message &out) ;
	void delete_resource (rmanager_list *r) ;
	void reset () ;

	void print (void) ;

    private :
	rmanager_list *resources_ ;

	rmanager_list *get_resource_instance (option *o) ;
} ;

#endif

