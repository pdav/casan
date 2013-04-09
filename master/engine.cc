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
 * are used to receive events from clients:
 * - events which can be matched with a request are handled through
 *   the request handler
 * - events which can not be paired with a request are handled through
 *   the client handler
 * - events which are not issued by a recognized client are ignored.
 */


#include <iostream>
#include <chrono>

#include <poll.h>
#define	NTAB(t)	(sizeof (t)/sizeof (t)[0])
#include <string.h>

#include "engine.h"

struct receiver
{
    l2net *l2 ;
    std::thread *thr ;
    receiver_t next ;
} ;

struct client
{
    l2net *l2 ;
    l2addr *addr ;
    req_handler_t handler ;
    client_t next ;
} ;

/*
 * Constructor and destructor
 */

engine::engine ()
{
    rlist = NULL ;
    clist = NULL ;
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
	std::lock_guard <std::mutex> lk (mut) ;
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

void engine::add_client (l2net *l2, l2addr *a, req_handler_t handler)
{
    std::lock_guard <std::mutex> lk (mut) ;
    client_t c ;

    c = new struct client ;
    c->l2 = l2 ;
    c->addr = a ;
    c->handler = handler ;
    c->next = clist ;

    clist = c ;
    condvar.notify_one () ;
}

void engine::add_request (request &r)
{
    std::lock_guard <std::mutex> lk (mut) ;

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
    std::unique_lock <std::mutex> lk (mut) ;
    std::cv_status cvstat ;

    for (;;)
    {
	cvstat = condvar.wait_for (lk, std::chrono::seconds (10)) ;
	if (cvstat == std::cv_status::timeout)
	{
	    std::cout << "TIMEOUT\n" ;
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
	     * Traverse client list to check new entries
	     */

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
    struct pollfd fds [1] ;
    int r ;

    fds [0].fd = l2->getfd () ;
    fds [0].events = POLLIN ;
    fds [0].revents = 0 ;

    for (;;)
    {
	r = poll (fds, NTAB (fds), 0) ;
	if (r == 1)
	{
	    std::cout << "received a packet on " << l2->getfd () << "\n" ;
	}
    }
}
