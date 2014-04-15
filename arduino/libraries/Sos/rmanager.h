#ifndef __RMANAGER_H__
#define __RMANAGER_H__

#include "option.h"
#include "msg.h"
#include "resource.h"

#define SOS_RESOURCES_ALL		"resources"

class Rmanager {
    public:
	Rmanager () ;
	~Rmanager () ;

	void add_resource (Resource *resource) ;
	void request_resource (Msg &in, Msg &out) ;
	void get_resource_list (Msg &out) ;
	void reset (void) ;

	void print (void) ;

    private :
	struct rmanager_list
	{
	    Resource *res ;
	    rmanager_list *next ;
	} ;

	rmanager_list *resources_ ;

	Resource *resource_by_name (const char *name, int namelen) ;
	void delete_resource (rmanager_list *r) ;
} ;

#endif
