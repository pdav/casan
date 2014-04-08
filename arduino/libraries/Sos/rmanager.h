#ifndef __RMANAGER_H__
#define __RMANAGER_H__

#include "option.h"
#include "msg.h"
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
	uint8_t request_resource (Msg &in, Msg &out) ;
	void get_all_resources (Msg &out) ;
	void delete_resource (rmanager_list *r) ;
	void reset () ;

	void print (void) ;

    private :
	rmanager_list *resources_ ;

	rmanager_list *get_resource_instance (option *o) ;
} ;

#endif

