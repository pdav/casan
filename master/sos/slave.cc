/**
 * @file slave.cc
 * @brief SOS slave class implementation
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

/**
 * @brief Reset a slave to a known state
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

/**
 * @brief Dumps slave status and resources to an output stream
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
	    os << "RUNNING (mtu=" << s.mtu_ << ", ttl=" << buf << ")" ;
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

/**
 * @brief Look-up the given resource
 *
 * Find a given resource /a/b/c on the slave. The resource is
 * represented as a vector of string (vector [] = { a, b, c}).
 *
 * @param v vector containing the components of the resource URL
 * @return pointer to resource, if found. NULL otherwise.
 */

resource *slave::find_resource (const std::vector <std::string> &v)
{
    resource *r = nullptr ;

    for (auto &res : reslist_)
    {
	if (res == v)
	{
	    r = &res ;
	    break ;
	}
    }
    return r ;
}

/**
 * @brief Fetches the resources advertised by the slave
 *
 * Fetches all the resources advertised by a slave in its
 * Assoc Answer message (its /.well-known/sos).
 *
 * @return vector of resource objects
 */

const std::vector <resource> &slave::resource_list (void)
{
    return reslist_ ;
}


/**
 * @brief SOS protocol handling
 *
 * This method is called by a receiver thread when the received
 * message is a control message originated from this slave.
 * The method implements the SOS control protocol for this
 * slave, and maintains the state associated to this slave.
 *
 * @param e current sos engine
 * @param m received message
 */

void slave::process_sos (sos *e, msgptr_t m)
{
    switch (m->sos_type ())
    {
	case msg::SOS_DISCOVER :
	    {
		D ("Received DISCOVER, sending ASSOCIATE") ;
		msgptr_t answer (new msg) ;
		answer->peer (m->peer ()) ;
		answer->type (msg::MT_CON) ;
		answer->code (msg::MC_POST) ;
		answer->mk_ctl_assoc (init_ttl_) ;
		e->add_request (answer) ;
	    }
	    break ;
	case msg::SOS_ASSOC_REQUEST :
	    D ("Received ASSOC_REQUEST from another master") ;
	    break ;
	case msg::SOS_ASSOC_ANSWER :
	    // has answer been correlated to a request?
	    if (m->reqrep ())
	    {
		byte *pload ; int plen ;
		std::vector <resource> rlist ;

		D ("Received ASSOC ANSWER for slave" << slaveid_) ;

		 // Add ressource list from the answer
		pload = (byte *) m->payload (&plen) ;
		D ("payload length=" << plen) ;

		if (parse_resource_list (rlist, pload, plen))
		{
		    D ("Slave " << slaveid_ << " status set to RUNNING") ;
		    status_ = SL_RUNNING ;
		    reslist_ = rlist ;
		    next_timeout_ = DATE_TIMEOUT_S (init_ttl_) ;
		}
		else
		    D ("Slave " << slaveid_ << " cannot parse resource list") ;
	    }
	    break ;
	case msg::SOS_HELLO :
	    D ("Received HELLO from another master") ;
	    break ;
	default :
	    D ("Received an unrecognized message") ;
	    break ;
    }
}

bool slave::parse_resource_list (std::vector <resource> &rlist, const byte *b, int len)
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
		    rlist.push_back (resource (current)) ;
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
		    rlist.back ().add_attribute (attrname, "") ;
		    attrname = "" ;
		    state = s_attrname ;	// stay in current state
		}
		else if (*b == ',')
		{
		    rlist.back ().add_attribute (attrname, "") ;
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
		    rlist.back ().add_attribute (attrname, "") ;
		    state = s_start ;
		}
		else if (*b == ';')
		{
		    rlist.back ().add_attribute (attrname, "") ;
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
		    rlist.back ().add_attribute (attrname, current) ;
		    state = s_endres ;
		}
		else current += *b ;
		break ;
	    case s_attrval_nquoted :
		if (*b == ',')
		{
		    rlist.back ().add_attribute (attrname, current) ;
		    state = s_start ;
		}
		else if (*b == ';')
		{
		    rlist.back ().add_attribute (attrname, current) ;
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
		rlist.back ().add_attribute (attrname, "") ;
	    break ;
	case s_attrval_start :
	    rlist.back ().add_attribute (attrname, "") ;
	    break ;
	case s_attrval_nquoted :
	    rlist.back ().add_attribute (attrname, current) ;
	    break ;
	default :
	    // nothing to do
	    state = s_error ;
	    break ;
    }

    return state != s_error ;
}

}					// end of namespace sos
