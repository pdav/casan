#ifndef SOS_SLAVE_H
#define	SOS_SLAVE_H

#include <mutex>

#include "l2.h"
#include "req.h"

class slave
{
    public:
	enum status
	{
	    SL_COLDSTART,
	    SL_WAITING_UNKNOWN,
	    SL_RUNNING,
	    SL_RENEW,
	    SL_WAITING_KNOWN,
	} ;

	slave () ;			// constructor
	~slave () ;			// destructor

	void reset (void) ;		// reset state to void
	void set_l2net (l2net *l2) ;	// set l2 network
	void set_l2addr (l2addr *a) ;	// set address
	void set_ttl (int ttl) ;	// set ttl
	void set_handler (req_handler_t h) ;// set handler for incoming requests

    protected:
	// process a message from this slave
	// data is NULL if this is timeout
	void process (void *data, int len) ;

	l2net *l2_ ;			// l2 network this slave is on
	l2addr *addr_ ;			// slave address

	friend class engine ;

    private:
	int ttl_ ;			// remaining ttl
	enum status status_ ;		// current status of slave
	req_handler_t handler_ ;	// handler to process answers
} ;

#endif
