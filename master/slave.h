#ifndef SOS_SLAVE_H
#define	SOS_SLAVE_H

#include <mutex>

#include "sos.h"
#include "l2.h"
#include "msg.h"

class slave
{
    public:
	enum status
	{
	    SL_INACTIVE,
	    SL_ASSOCIATING,
	    SL_RUNNING,
	} ;

	slave () ;			// constructor
	~slave () ;			// destructor

	void reset (void) ;		// reset state to void

	// Mutators
	void l2 (l2net *l2) ;		// set l2 network
	void addr (l2addr *a) ;		// set address
	void ttl (int ttl) ;		// set ttl
	void slaveid (slaveid_t sid) ;	// set slave id

	// Accessors
	l2net *l2 (void) ;
	l2addr *addr (void) ;
	int ttl (void) ;
	slaveid_t slaveid (void) ;

	void handler (reply_handler_t h) ;// handler for incoming requests

    protected:
	// process a message from this slave
	// data is NULL if this is timeout
	void process (void *data, int len) ;

	slaveid_t slaveid_ ;		// slave id
	enum status status_ ;		// current status of slave
	reply_handler_t handler_ ;	// handler to process answers

	friend class engine ;

    private:
	l2net *l2_ ;			// l2 network this slave is on
	l2addr *addr_ ;			// slave address
	int ttl_ ;			// remaining ttl
} ;

#endif
