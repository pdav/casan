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
    std::thread *thr ;
    receiver_t next ;
} ;

/*
 * Constructor and destructor
 */

engine::engine ()
{
    rlist = NULL ;
    tsender = NULL ;
}

engine::~engine ()
{
    while (rlist != NULL)
    {
	receiver_t next ;

	next = rlist->next ;
	delete rlist ;
	rlist = next ;
    }
    slist.clear () ;
    qlist.clear () ;
}

void engine::init (void)
{
    if (tsender == NULL)
    {
	tsender = new std::thread (&engine::sender, this) ;
    }
}

void engine::start_net (l2net *l2)
{
    if (tsender != NULL)
    {
	std::lock_guard <std::mutex> lk (mtx) ;
	receiver_t r ;

	r = new struct receiver ;
	r->l2 = l2 ;
	r->thr = NULL ;			// not started

	r->next = rlist ;
	rlist = r ;
	condvar.notify_one () ;
    }
}

void engine::stop_net (l2net *l2)
{
}

void engine::add_slave (slave &s)
{
    std::lock_guard <std::mutex> lk (mtx) ;

    slist.push_front (s) ;
    condvar.notify_one () ;
}

void engine::add_request (request &r)
{
    std::lock_guard <std::mutex> lk (mtx) ;

    qlist.push_front (r) ;
    condvar.notify_one () ;
}

/*
 * Sender thread
 * Block on a condition variable, waiting for events:
 * - timer event, signalling that a request timeout has expired
 * - change in L2 network handling (new L2, or removed L2)
 * - a new request is added (from an exterior thread)
 * - answer for a paired request has been received (from a receiver thread)
 */

void engine::sender (void)
{
    std::unique_lock <std::mutex> lk (mtx) ;
    std::cv_status cvstat ;

    for (;;)
    {
	cvstat = condvar.wait_for (lk, std::chrono::seconds (10)) ;
	if (cvstat == std::cv_status::timeout)
	{
	    std::cout << "TIMEOUT\n" ;
	    /*
	     * Send hello
	     */

	    receiver_t r ;
	    char buf [] = "coucou++ !" ;
	    for (r = rlist ; r != NULL ; r = r->next)
		r->l2->bsend (buf, strlen (buf)) ;
	}
	else
	{
	    /*
	     * Traverse the receiver list to check new entries
	     */

	    receiver_t r ;
	    for (r = rlist ; r != NULL && r->thr == NULL ; r = r->next)
	    {
		std::cout << "Found a receiver to start\n" ;
		r->thr = new std::thread (&engine::receiver, this, r->l2) ;
	    }

	    /*
	     * Traverse slave list to check new entries
	     */

	    std::list <slave>::iterator s ;

	    for (s = slist.begin () ; s != slist.end () ; s++)
	    {
	    }

	    /*
	     * Traverse request list to check new entries
	     */

	    std::list <request>::iterator q ;

	    for (q = qlist.begin () ; q != qlist.end () ; q++)
	    {
		if (q->status == REQ_NONE)
		{
		    struct message
		    {
			int mid ;
			char text [10] ;
		    } m ;

		    std::cout << "Found a request to handle\n" ;
		    q->status = REQ_WAITING ;
		    m.mid = q->id ;
		    strcpy (m.text, "coucou !\n") ;
		    
		    if (q->dl2->send (q->daddr, &m, sizeof m) == -1)
		    {
			perror ("SEND") ;
		    }
		}
	    }
	}
    }
}

/*
 * Receiver thread
 * Block on packet reception on the given interface
 */

void engine::receiver (l2net *l2)
{
    l2addr *a ;
    int len ;
    pktype_t pkt ;
    char data [MAXBUF] ;
    int found ;

    for (;;)
    {
	len = sizeof data ;
	pkt = l2->recv (&a, data, &len) ;	// create a l2addr *a
	std::cout << "pkt=" << pkt << ", len=" << len << std::endl ;

	/*
	 * Search originator slave
	 */

	std::list <slave>::iterator s ;

	found = 0 ;
	for (s = slist.begin () ; s != slist.end () ; s++)
	{
	    if (*a == *(s->addr_))
	    {
		found = 1 ;
		break ;
	    }
	}
	delete a ;			// remove address created by recv()

	/*
	 * If originator slave is found, process the request.
	 * Else, ignore it.
	 */

	if (found)
	{
	    /*
	     * Process request
	     */

	    std::cout << "slave found" << std::endl ;
	}
    }
}
