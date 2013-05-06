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
#include <cstdio>
#include <cstring>

#include "engine.h"
#include "sos.h"
#include "utils.h"

#include "defs.h"

struct receiver
{
    l2net *l2 ;
    long int hid ; 		// hello id, initialized at start time
    slave broadcast ;
    std::list <msg *> deduplist ;	// for received messages
    std::chrono::system_clock::time_point next_hello ;
    std::thread *thr ;
} ;



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
	// std::memset (r, 0, sizeof *r) ;

	r->l2 = l2 ;
	r->thr = NULL ;
	// define a pseudo-slave for broadcast address
	r->broadcast.l2 (l2) ;
	r->broadcast.addr (l2->bcastaddr ()) ;

	now = std::chrono::system_clock::now () ;
	r->hid = std::chrono::system_clock::to_time_t (now) ;
	r->next_hello = now + random_timeout (DELAY_FIRST_HELLO)  ;

	rlist_.push_front (*r) ;
	condvar_.notify_one () ;
    }
}

void engine::stop_net (l2net *l2)
{
    std::cout << sizeof *l2 << "\n" ;	// calm down -Wall
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
}

/******************************************************************************
 * Add a new message to send
 */

void engine::add_request (msg *m)
{
    std::lock_guard <std::mutex> lk (mtx_) ;

    mlist_.push_front (m) ;
    condvar_.notify_one () ;
}

/******************************************************************************
 * SOS autodiscovery handling
 */

void engine::send_hello (receiver *r)
{
    msg m ;
    char buf [MAXBUF] ;

    m.peer (& r->broadcast) ;		// send to broadcast address
    m.type (msg::MT_NON) ;
    m.code (msg::MC_POST) ;
    snprintf (buf, MAXBUF, "POST /.well-known/sos?uuid=%ld", r->hid) ;
    m.payload (buf, strlen (buf)) ;

    D ("Send Hello") ;
    m.send () ;
}

/******************************************************************************
 * Sender thread
 * Block on a condition variable, waiting for events:
 * - timer event, signalling that a request timeout has expired
 * - change in L2 network handling (new L2, or removed L2)
 * - a new request is added (from an exterior thread)
 */

void engine::sender_thread (void)
{
    std::unique_lock <std::mutex> lk (mtx_) ;
    std::chrono::system_clock::time_point now, next_timeout ;

    now = std::chrono::system_clock::now () ;
    next_timeout = std::chrono::system_clock::time_point::max () ;

    for (;;)
    {
	std::list <receiver>::iterator ri ;
	std::list <msg *>::iterator mi ;

	if (next_timeout == std::chrono::system_clock::time_point::max ())
	{
	    D ("WAIT") ;
	    condvar_.wait (lk) ;
	}
	else
	{
	    auto delay = next_timeout - now ;	// needed precision for delay

	    D ("WAIT " << std::chrono::duration_cast<std::chrono::milliseconds> (delay).count() << "ms") ;
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

	for (ri = rlist_.begin () ; ri != rlist_.end () ; ri++)
	{
	    // should the receiver thread be started?
	    if (ri->thr == NULL)
	    {
		D ("Found a receiver to start") ;
		ri->thr = new std::thread (&engine::receiver_thread, this, &*ri) ;
	    }

	    // is it time to send a new hello ?
	    if (now >= ri->next_hello)
	    {
		engine::send_hello (&*ri) ;

		// schedule next hello packet
		ri->next_hello = now + std::chrono::milliseconds (INTERVAL_HELLO) ;
	    }
	}

	/*
	 * Traverse message list to check new messages to send or older
	 * messages to retransmit
	 */

	mi = mlist_.begin () ;
	while (mi != mlist_.end ())
	{
	    auto newmi = mi ;		// backup mi since we may erase it
	    newmi++ ;

	    /*
	     * New message to transmit or old message to retransmit
	     */

	    if ((*mi)->ntrans_ == 0 ||
		    ((*mi)->ntrans_ < MAX_RETRANSMIT && now >= (*mi)->next_timeout_)
		    )
	    {
		D ("Found a " << ((*mi)->ntrans_==0?"new ":"") << "request to handle") ;
		if ((*mi)->send () == -1)
		{
		    std::cout << "ERROR DURING TRANSMISSION\n" ;
		}
	    }

	    /*
	     * Message expired
	     */

	    if (now > (*mi)->expire_)
	    {
		D ("ERASE id=" << (*mi)->id ()) ;
		delete *mi ;
		mlist_.erase (mi) ;
	    }

	    mi = newmi ;
	}

	/*
	 * Determine date of next action
	 */

	now = std::chrono::system_clock::now () ;

	for (auto &r : rlist_)
	{
	    // must current timeout be the next hello?
	    if (next_timeout > r.next_hello)
		next_timeout = r.next_hello ;
	}

	for (auto &m : mlist_)
	{
	    // must current timeout be the next retransmission of this message?
	    if (next_timeout > m->next_timeout_)
		next_timeout = m->next_timeout_ ;
	}
    }
}

/******************************************************************************
 * Receiver thread
 * Block on packet reception on the given interface
 */

bool engine::find_peer (msg *m, l2addr *a, receiver *r)
{
    bool found ;

    found = false ;
    if (a)
    {
	bool free_a = true ;

	/*
	 * Is the peer already known?
	 */

	for (auto &s : slist_)
	{
	    if (*a == *(s.addr ()))
	    {
		m->peer (&s) ;
		found = true ;
		break ;
	    }
	}

	/*
	 * If the peer is not known, it may be a new slave coming up
	 * In this case, we must send a ASSOCIATE message
	 */

	if (!found)
	{
	    slaveid_t sid ;

	    sid = m->is_sos_register () ;
	    if (sid)
	    {
		for (auto &s : slist_)
		{
		    if (sid == s.slaveid_)
		    {
			s.l2 (r->l2) ;
			s.addr (a) ; free_a = false ;
			m->peer (&s) ;
			found = true ;
			break ;
		    }
		}
	    }
	}

	if (free_a)
	    delete a ;
    }

    return found ;
}

void engine::clean_deduplist (receiver *r)
{
    std::list <msg *>::iterator di ;
    std::chrono::system_clock::time_point now ;

    now = std::chrono::system_clock::now () ;

    di = r->deduplist.begin () ;
    while (di != r->deduplist.end ())
    {
	auto newdi = di ;		// backup di since we may erase it
	newdi++ ;

	if (now >= (*di)->expire_)
	{
	    D ("ERASE FROM DEDUP id=" << (*di)->id ()) ;
	    r->deduplist.erase (di) ;
	}

	di = newdi ;
    }
}

void engine::receiver_thread (receiver *r)
{
    for (;;)
    {
	msg *m ;
	l2addr *a ;
	bool process_it ;
	msg *orgmsg ;			// original message in case of duplicate

	/*
	 * Wait for a new message
	 */

	m = new msg ;
	a = m->recv (r->l2) ;
	process_it = true ;

	/*
	 * House cleaning: remove obsolete messages from deduplication list
	 */

	clean_deduplist (r) ;

	/*
	 * Find slave
	 */

	if (! find_peer (m, a, r))
	{
	    process_it = false ;
	    continue ;
	}

	/*
	 * Message correlation: see CoAP spec, section 4.4
	 * Is this a reply to an existing request?
	 * For this, we need to perform a search in the message list
	 * for a request message with this id.
	 */

	if (process_it && (m->type () == msg::MT_ACK || m->type () == msg::MT_RST))
	{
	    std::lock_guard <std::mutex> lk (mtx_) ;
	    int id ;

	    id = m->id () ;
	    for (auto &mr : mlist_)
	    {
		if (mr->id () == id)
		{
		    /* Got it! Original request found */
		    if (mr->reqrep ())
		    {
			// answer already received : ignore this new message
			process_it = false ;
		    }
		    else
		    {
			// new (unseen) answer to the original message
			// register 
			mr->link_reqrep (m) ;
			// no more re-transmission needed
			mr->complete () ;
		    }
		    break ;
		}
	    }
	}

	/*
	 * Message deduplication: see CoAP spec, section 4.5
	 * Is this the same request as already seen?
	 */

	orgmsg = 0 ;			// no duplicate
	if (process_it && (m->type () == msg::MT_CON || m->type () == msg::MT_NON))
	{
	    for (auto &d : r->deduplist)
	    {
		if (*m == *d)
		{
		    orgmsg = *(&d) ;
		    break ;
		}
	    }
	    if (orgmsg)
	    {
		/* found a duplicated message */
		D ("DUPLICATE MESSAGE id=" << orgmsg->id ()) ;
	    }
	    else
	    {
		// store the new message on deduplist with a timeout
		m->expire_ = DATE_TIMEOUT (EXCHANGE_LIFETIME (r->l2->maxlatency ())) ;
		r->deduplist.push_front (m) ;
	    }
	}

	/*
	 * Check SOS control messages first
	 */

	if (process_it && m->sos_type () != msg::SOS_NONE)
	{
		D ("CTL MSG") ;
		delete m ;
		process_it = false ;		// already processed
	}

	if (process_it && m->peer ()->status_ == slave::SL_RUNNING)
	{
	    /*
	     * Is this message a duplicated one?
	     */

	    if (m->type () == msg::MT_CON)
	    {
		////         XXXXXXXXXXXXXXXXXXXXX
	    }
	}
    }
}
