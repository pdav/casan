/*
 * Main engine of the SOS system
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
#include <cstdlib>

#include <string.h>

#include "engine.h"

#include "../include/defs.h"

#define	MAXBUF	1024

struct receiver
{
    l2net *l2 ;
    long int hid ; 		// hello id, initialized at start time
    std::chrono::system_clock::time_point next_hello ;
    std::thread *thr ;
} ;



/******************************************************************************
 * Some utility functions
 */

/*
 * returns a random delay between 0 and maxmilli milliseconds
 */

std::chrono::milliseconds random_timeout (int maxmilli)
{
    int delay ;

    delay = std::rand () % (maxmilli + 1) ;
    return std::chrono::milliseconds (delay) ;
}

/******************************************************************************
 * Constructor and destructor
 */

engine::engine ()
{
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
    std::srand (std::time (0)) ;

    if (tsender_ == NULL)
    {
	tsender_ = new std::thread (&engine::sender_thread, this) ;
    }
}

/******************************************************************************
 * Add a new L2 network:
 * - schedule the first HELLO packet
 * - add the network to the receiver queue
 * - notifiy the sender thread in order to create a new receiver thread
 */

void engine::start_net (l2net *l2)
{
    if (tsender_ != NULL)
    {
	std::lock_guard <std::mutex> lk (mtx_) ;
	receiver *r ;
	std::chrono::system_clock::time_point now ;

	r = new receiver ;

	r->l2 = l2 ;
	r->thr = NULL ;

	now = std::chrono::system_clock::now () ;
	r->hid = std::chrono::system_clock::to_time_t (now) ;
	r->next_hello = now + random_timeout (DELAY_FIRST_HELLO)  ;

	rlist_.push_front (*r) ;
	condvar_.notify_one () ;
    }
}

void engine::stop_net (l2net *l2)
{
}

/******************************************************************************
 * Add a new slave:
 * - initialize the status
 * - add it to the slave list
 * - (no need to notify the sender thread)
 */

void engine::add_slave (slave *s)
{
    std::lock_guard <std::mutex> lk (mtx_) ;

    s->status_ = slave::SL_INACTIVE ;
    slist_.push_front (*s) ;
    // condvar_.notify_one () ;
}

/******************************************************************************
 * Add a new message to send
 */

void engine::add_request (msg *m)
{
    std::lock_guard <std::mutex> lk (mtx_) ;

    // timeout_ = 5 ;
    mlist_.push_front (*m) ;
    condvar_.notify_one () ;
}

/******************************************************************************
 * Sender thread
 * Block on a condition variable, waiting for events:
 * - timer event, signalling that a request timeout has expired
 * - change in L2 network handling (new L2, or removed L2)
 * - a new request is added (from an exterior thread)
 * - answer for a paired request has been received (from a receiver thread)
 */

void engine::sender_thread (void)
{
    std::unique_lock <std::mutex> lk (mtx_) ;
    std::cv_status cvstat ;
    std::chrono::system_clock::time_point now, next_timeout ;

    next_timeout = std::chrono::system_clock::time_point::max () ;

    for (;;)
    {
	std::cout << "WAIT\n" ;
	if (next_timeout == std::chrono::system_clock::time_point::max ())
	{
	    condvar_.wait (lk) ;
	}
	else
	{
	    auto delay = next_timeout - now ;	// needed precision for delay

	    condvar_.wait_for (lk, delay) ;
	}

	/*
	 * we have been awaken for (one or more) multiple reasons:
	 * - a new l2 network has been registered
	 * - a new slave has been registered
	 * - a new message is to be sent
	 * - timeout expired: there is an action to do (message to
	 *	retransmit or to remove from a queue)
	 *
	 * After each iteration, we check all timeout reasons.
	 */

	now = std::chrono::system_clock::now () ;
	next_timeout = std::chrono::system_clock::time_point::max () ;

	/*
	 * Traverse the receiver list to check new entries, and
	 * start receiver threads.
	 */

	std::list <receiver>::iterator r ;

	for (r = rlist_.begin () ; r != rlist_.end () ; r++)
	{
	    // should the receiver thread be started?
	    if (r->thr == NULL)
	    {
		std::cout << "Found a receiver to start\n" ;
		r->thr = new std::thread (&engine::receiver_thread, this, &*r) ;
	    }

	    // is it time to send a new hello ?
	    if (now >= r->next_hello)
	    {
		/*
		 * Send hello
		 */

		std::cout << "ENVOYER UN HELLO\n" ;

		// schedule next hello packet
		r->next_hello = now + std::chrono::milliseconds (INTERVAL_HELLO) ;
	    }

	    // must current timeout be the next hello?
	    if (next_timeout > r->next_hello)
		next_timeout = r->next_hello ;
	}

	/*
	 * Traverse message list to check new messages to send or older
	 * messages to retransmit
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

/******************************************************************************
 * Receiver thread
 * Block on packet reception on the given interface
 */

void engine::receiver_thread (receiver *r)
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
