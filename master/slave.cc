/*
 * Slave representation
 *
 * Each slave has a unique representation in the SOS system.
 */

#include <iostream>
#include <cstring>

#include "slave.h"
#include "engine.h"

#define	SET_INACTIVE	do { \
			    status_ = SL_INACTIVE ; \
			    next_timeout_ = std::chrono::system_clock::time_point::max () ; \
			} while (false)			// no ";"
/*
 * Constructor and destructor
 */

void slave::reset (void)
{
    if (addr_)
	delete addr_ ;			// XXX should be lock-protected
    l2_ = 0 ;
    addr_ = 0 ;
    SET_INACTIVE ;
}

/******************************************************************************
 * Accessors
 */

l2net *slave::l2 (void)
{
    return l2_ ;
}

l2addr *slave::addr (void)
{
    return addr_ ;
}

slaveid_t slave::slaveid (void)
{
    return slaveid_ ;
}

/******************************************************************************
 * Mutators
 */

void slave::l2 (l2net *l2)
{
    l2_ = l2 ;
}

void slave::addr (l2addr *a)
{
    addr_ = a ;
}

void slave::slaveid (slaveid_t sid)
{
    slaveid_ = sid ;
}

void slave::handler (reply_handler_t h)
{
    handler_ = h ;
}

/******************************************************************************
 * SOS protocol handling
 *
 * e: current engine
 * m: received message
 */

void slave::process_sos (engine *e, msg *m)
{
    char buf [MAXBUF] ;
    msg *answer = 0 ;

    switch (m->sos_type ())
    {
	case msg::SOS_REGISTER :
	    D ("Received REGISTER, sending ASSOCIATE") ;
	    answer = new msg ;
	    answer->peer (m->peer ()) ;
	    answer->type (msg::MT_CON) ;
	    answer->code (msg::MC_POST) ;
	    snprintf (buf, MAXBUF, "POST /.well-known/sos?associate=%ld", e->ttl ()) ;
	    answer->payload (buf, std::strlen (buf)) ;
	    e->add_request (answer) ;
	    break ;
	case msg::SOS_ASSOC_REQUEST :
	    D ("Received ASSOC_REQUEST from another master") ;
	    break ;
	case msg::SOS_ASSOC_ANSWER :
	    D ("Received ASSOC ANSWER") ;
	    // has answer been correlated to a request?
	    if (m->reqrep ())
	    {
		/*
		 * ADD RESOURCES FROM THE ANSWER
		 */
		status_ = SL_RUNNING ;
		next_timeout_ = DATE_TIMEOUT (e->ttl ()) ;
	    }
	    break ;
	case msg::SOS_HELLO :
	    D ("Received HELLO from another master") ;
	    break ;
	default :
	    D ("Received an unrecognized message") ;
	    break ;
    }
    delete m ;
}


/*
 * Process an event: either a message or a timeout from the slave
 */

void slave::process (void *data, int len)
{
    if (data)
    {
	// regular message
	std::cout << "received a message of len " << len << " bytes\n" ;
    }
    else
    {
	// timeout
	std::cout << "got a timeout\n" ;
    }
}
