/*
 * Slave representation
 *
 * Each slave has a unique representation in the SOS system.
 */

#include <iostream>
#include <cstring>
#include <vector>
#include <list>

#include <ctime>
#include <chrono>
#include <iomanip>			// put_tiime

#include "global.h"

#include "l2.h"
#include "msg.h"
#include "resource.h"
#include "slave.h"
#include "sos.h"

namespace sos {

/*
 * Constructor and destructor
 */

void slave::reset (void)
{
    if (addr_)
	delete addr_ ;			// XXX should be lock-protected
    l2_ = 0 ;
    addr_ = 0 ;
    reslist_.clear () ;
    status_ = SL_INACTIVE ;
    next_timeout_ = std::chrono::system_clock::time_point::max () ;
    D ("Slave " << slaveid_ << " status set to INACTIVE") ;
}

/******************************************************************************
 * Debug
 */

std::ostream& operator<< (std::ostream &os, const slave &s)
{
    l2addr *a ;
    std::time_t nt ;
    char buf [MAXBUF] ;

    os << "slave " << s.slaveid_ << " " ;
    switch (s.status_)
    {
	case slave::SL_INACTIVE:
	    os << "INACTIVE" ;
	    break ;
	case slave::SL_RUNNING:
	    nt = std::chrono::system_clock::to_time_t (s.next_timeout_) ;
	    strftime (buf, sizeof buf, "%F %T", std::localtime (&nt)) ;
	    os << "RUNNING (ttl=" << buf << ")" ;
	    break ;
	default:
	    os << "(unknown state)" ;
	    break ;
    }
    a = ((slave *) (&s))->addr () ;	// why "a = s.addr ()" doesn't work?
    if (a)
	os << " mac=" << *a ;
    os << "\n" ;

    // Display resources
    for (auto &r : s.reslist_)
	os << r ;

    return os ;
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

enum slave::status_code slave::status (void)
{
    return status_ ;
}

int slave::initttl (void)
{
    return initttl_ ;
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

void slave::initttl (int t)
{
    initttl_ = t ;
}

void slave::handler (reply_handler_t h)
{
    handler_ = h ;
}

/******************************************************************************
 * SOS protocol handling
 *
 * e: current sos engine
 * m: received message
 */

void slave::process_sos (sos *e, msg *m)
{
    msg *answer = 0 ;

    switch (m->sos_type ())
    {
	case msg::SOS_REGISTER :
	    D ("Received REGISTER, sending ASSOCIATE") ;
	    answer = new msg ;
	    answer->peer (m->peer ()) ;
	    answer->type (msg::MT_CON) ;
	    answer->code (msg::MC_POST) ;
	    answer->mk_ctl_assoc (initttl_) ;
	    e->add_request (answer) ;
	    break ;
	case msg::SOS_ASSOC_REQUEST :
	    D ("Received ASSOC_REQUEST from another master") ;
	    break ;
	case msg::SOS_ASSOC_ANSWER :
	    // has answer been correlated to a request?
	    if (m->reqrep ())
	    {
		byte *pload ; int plen ;

		D ("Received ASSOC ANSWER for slave" << slaveid_) ;

		 // Add ressource list from the answer
		pload = (byte *) m->payload (&plen) ;
		(void) parse_resource_list (pload, plen) ;

		D ("Slave " << slaveid_ << " status set to RUNNING") ;
		status_ = SL_RUNNING ;
		next_timeout_ = DATE_TIMEOUT_S (initttl_) ;
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

bool slave::parse_resource_list (const byte *b, int len)
{
    enum { s_start,			// potential terminal state
	    s_resource,
	    s_endres,			// potential terminal state
	    s_attrname,			// potential terminal state
	    s_attrval_start,		// potential terminal state
	    s_attrval_nquoted,		// potential terminal state
	    s_attrval_quoted,
	    s_error
	} state = s_start ;
    std::string current ;		// string currently being built
    std::string attrname ;		// attribute name

    // Ref: RFC 6690

    // XXX : this code does not check syntax of link-param and values
    // XXX : this code does nothing with link-param and values

    for ( ; len > 0 ; len--, b++)
    {
	switch (state)
	{
	    case s_start :
		if (*b == '<')
		{
		    current = "" ;
		    state = s_resource ;
		}
		else state = s_error ;
		break ;
	    case s_resource :
		if (*b == '>')
		{
		    reslist_.push_back (resource (current)) ;
		    state = s_endres ; 
		}
		else current += *b ;
		break ;
	    case s_endres :
		attrname = "" ;
		if (*b == ';')
		    state = s_attrname ;
		else if (*b == ',')
		    state = s_start ;
		else state = s_error ;
		break ;
	    case s_attrname :
		if (*b == '=')
		{
		    if (attrname == "")
			state = s_error ;
		    else
			state = s_attrval_start ;
		}
		else if (*b == ';')
		{
		    reslist_.back ().add_attribute (attrname, "") ;
		    attrname = "" ;
		    state = s_attrname ;	// stay in current state
		}
		else if (*b == ',')
		{
		    reslist_.back ().add_attribute (attrname, "") ;
		    state = s_start ;
		}
		else attrname += *b ;
		break ;
	    case s_attrval_start :
		current = "" ;
		if (*b == '"')
		    state = s_attrval_quoted ;
		else if (*b == ',')
		{
		    reslist_.back ().add_attribute (attrname, "") ;
		    state = s_start ;
		}
		else if (*b == ';')
		{
		    reslist_.back ().add_attribute (attrname, "") ;
		    attrname = "" ;
		    state = s_attrname ;
		}
		else
		{
		    state = s_attrval_nquoted ;
		    current += *b ;
		}
		break ;
	    case s_attrval_quoted :
		if (*b == '"')
		{
		    reslist_.back ().add_attribute (attrname, current) ;
		    state = s_endres ;
		}
		else current += *b ;
		break ;
	    case s_attrval_nquoted :
		if (*b == ',')
		{
		    reslist_.back ().add_attribute (attrname, current) ;
		    state = s_start ;
		}
		else if (*b == ';')
		{
		    reslist_.back ().add_attribute (attrname, current) ;
		    attrname = "" ;
		    state = s_attrname ;
		}
		else current += *b ;
		break ;
	    default :
		state = s_error ;
		break ;
	}
    }
    
    // take care of terminal states
    switch (state)
    {
	case s_start :
	    // nothing to do
	    break ;
	case s_endres :
	    break ;
	case s_attrname :
	    if (attrname == "")
		reslist_.back ().add_attribute (attrname, "") ;
	    break ;
	case s_attrval_start :
	    reslist_.back ().add_attribute (attrname, "") ;
	    break ;
	case s_attrval_nquoted :
	    reslist_.back ().add_attribute (attrname, current) ;
	    break ;
	default :
	    // nothing to do
	    state = s_error ;
	    break ;
    }

    return state != s_error ;
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

}					// end of namespace sos
