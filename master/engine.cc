/*
 * Sender of the SOS system
 *
 * This is the sender thread of the SOS system: it processes send
 * requests, eventually re-transmits requests with a timeout.
 * Other threads send requests by:
 * - adding a request in the queue, and
 * - setting the sender condition variable and signalling this thread
 *
 * There is a receiver thread by L2 network. These receiver threads
 * are used to receive events from slaves:
 * - events which can be matched with a request are handled through
 *   the request handler
 * - events which can not be paired with a request are handled through
 *   the slave handler
 * - events which are not issued by a recognized slave are ignored.
 */


#include <iostream>
#include <chrono>

#include <string.h>

#include "engine.h"

#include "../include/defs.h"

#define	MAXBUF	1024

struct receiver
{
    l2net *l2 ;
    std::chrono::system_clock::time_point last_hello ;
    std::thread *thr ;
} ;

/*
 * Constructor and destructor
 */

engine::engine ()
{
    std::chrono::system_clock::time_point now ;

    now = std::chrono::system_clock::now () ;
    hid_ = std::chrono::system_clock::to_time_t (now) ;

    // rlist_ = NULL ;
    tsender_ = NULL ;
}

engine::~engine ()
{
    rlist_.clear () ;
    slist_.clear () ;
    mlist_.clear () ;
}

void engine::init (void)
{
    if (tsender_ == NULL)
    {
	tsender_ = new std::thread (&engine::sender, this) ;
    }
}

void engine::start_net (l2net *l2)
{
    if (tsender_ != NULL)
    {
	std::lock_guard <std::mutex> lk (mtx_) ;
	receiver *r ;

	r = new receiver ;
	r->l2 = l2 ;
	r->thr = NULL ;
	rlist_.push_front (*r) ;
	condvar_.notify_one () ;
    }
}

void engine::stop_net (l2net *l2)
{
}

void engine::add_slave (slave &s)
{
    std::lock_guard <std::mutex> lk (mtx_) ;

    slist_.push_front (s) ;
    condvar_.notify_one () ;
}

void engine::add_request (msg &m)
{
    std::lock_guard <std::mutex> lk (mtx_) ;

    mlist_.push_front (m) ;
    condvar_.notify_one () ;
}

/******************************************************************************
 * Utility functions
 */

std::chrono::system_clock::time_point engine::next_timeout (void)
{
    std::chrono::system_clock::time_point next ;
    std::list <receiver>::iterator r ;

    next = std::chrono::system_clock::time_point::max () ;

    /*
     * HELLO message on each network
     */

    for (r = rlist_.begin () ; r != rlist_.end () ; r++)
    {
	std::chrono::system_clock::time_point h ;

	h = r->last_hello + std::chrono::milliseconds (INTERVAL_HELLO) ;
	if (next > h)
	    next = h ;
    }

    return next ;
}

/******************************************************************************
 * Sender thread
 * Block on a condition variable, waiting for events:
 * - timer event, signalling that a request timeout has expired
 * - change in L2 network handling (new L2, or removed L2)
 * - a new request is added (from an exterior thread)
 * - answer for a paired request has been received (from a receiver thread)
 */

void engine::sender (void)
{
    std::unique_lock <std::mutex> lk (mtx_) ;
    std::cv_status cvstat ;

    for (;;)
    {
	cvstat = condvar_.wait_for (lk, std::chrono::seconds (10)) ;
	if (cvstat == std::cv_status::timeout)
	{
	    std::cout << "TIMEOUT\n" ;
	    /*
	     * Send hello
	     */

	    std::list <receiver>::iterator r ;
	    char buf [] = "coucou++ !" ;

	    for (r = rlist_.begin () ; r != rlist_.end () ; r++)
		r->l2->bsend (buf, strlen (buf)) ;
	}
	else
	{
	    /*
	     * Traverse the receiver list to check new entries
	     */

	    std::list <receiver>::iterator r ;

	    for (r = rlist_.begin () ; r != rlist_.end () ; r++)
	    {
		if (r->thr == NULL)
		{
		    std::cout << "Found a receiver to start\n" ;
		    r->thr = new std::thread (&engine::receive, this, &(*r)) ;
		}
	    }

	    /*
	     * Traverse slave list to check new entries
	     */

	    std::list <slave>::iterator s ;

	    for (s = slist_.begin () ; s != slist_.end () ; s++)
	    {
	    }

	    /*
	     * Traverse request list to check new entries
	     */

	    std::list <msg>::iterator m ;

	    for (m = mlist_.begin () ; m != mlist_.end () ; m++)
	    {
		if (m->ntrans_ == 0)
		{
		    std::cout << "Found a request to handle\n" ;
		    m->send () ;
		    /***** TESTER ERREUR *****/
		}
	    }
	}
    }
}

/******************************************************************************
 * Receiver thread
 * Block on packet reception on the given interface
 */

void engine::receive (receiver *r)
{
    l2addr *a ;
    int len ;
    pktype_t pkt ;
    char data [MAXBUF] ;
    int found ;

    for (;;)
    {
	len = sizeof data ;
	pkt = r->l2->recv (&a, data, &len) ;	// create a l2addr *a
	std::cout << "pkt=" << pkt << ", len=" << len << std::endl ;

	/*
	 * Search originator slave
	 */

	std::list <slave>::iterator s ;

	found = 0 ;
	for (s = slist_.begin () ; s != slist_.end () ; s++)
	{
	    if (*a == *(s->addr_))
	    {
		found = 1 ;
		break ;
	    }
	}
	delete a ;			// remove address created by recv()

	if (! found)
	    continue ;			// ignore message if no slave is found

	/*
	 * Process request
	 */

	std::cout << "slave found" << std::endl ;

	// create a message from raw data
	// is this a already received message (deduplication)?
	// is this a reply to an existing request?
    }
}
