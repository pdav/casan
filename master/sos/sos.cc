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
#include <sstream>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cstring>

// don't define NDEBUG
#include <cassert>

#include "global.h"
#include "utils.h"

#include "l2.h"
#include "msg.h"
#include "resource.h"
#include "sos.h"

namespace sos {

struct receiver
{
    l2net *l2 ;
    long int hid ; 			// hello id, initialized at start time
    slave broadcast ;
    std::list <msg *> deduplist ;	// received messages
    msg *hellomsg ;
    timepoint_t next_hello ;
    std::thread *thr ;
} ;



/******************************************************************************
 * Constructor and destructor
 */

sos::sos ()
{
    tsender_ = NULL ;
}

sos::~sos ()
{
    rlist_.clear () ;
    slist_.clear () ;
    mlist_.clear () ;
}

void sos::init (void)
{
    std::srand (std::time (0)) ;

    if (tsender_ == NULL)
    {
	tsender_ = new std::thread (&sos::sender_thread, this) ;
    }
}

void sos::ttl (slavettl_t t)
{
    ttl_ = t ;
}

slavettl_t sos::ttl (void)
{
    return ttl_ ;
}

/******************************************************************************
 * Displays (in a string) dynamic data structures
 */

std::string sos::html_debug (void)
{
    std::ostringstream oss ;

    oss << "Default TTL = " << ttl_ << "\n" ;
    oss << "Slaves:\n" ;
    for (auto &s : slist_)
	oss << s ;

    return oss.str () ;
}

/******************************************************************************
 * Add a new L2 network:
 * - schedule the first HELLO packet
 * - add the network to the receiver queue
 * - notifiy the sender thread in order to create a new receiver thread
 */

void sos::start_net (l2net *l2)
{
    if (tsender_ != NULL)
    {
	std::lock_guard <std::mutex> lk (mtx_) ;
	receiver *r ;
	timepoint_t now ;

	r = new receiver ;
	// std::memset (r, 0, sizeof *r) ;

	r->l2 = l2 ;
	r->thr = NULL ;
	// define a pseudo-slave for broadcast address
	r->broadcast.l2 (l2) ;
	r->broadcast.addr (l2->bcastaddr ()) ;

	now = std::chrono::system_clock::now () ;
	r->hid = std::chrono::system_clock::to_time_t (now) % 1000 ;

	r->hellomsg = new msg () ;
	r->hellomsg->peer (& r->broadcast) ;
	r->hellomsg->type (msg::MT_NON) ;
	r->hellomsg->code (msg::MC_POST) ;
	r->hellomsg->mk_ctl_hello (r->hid) ;

	r->next_hello = now + random_timeout (DELAY_FIRST_HELLO)  ;

	rlist_.push_front (*r) ;
	condvar_.notify_one () ;
    }
}

void sos::stop_net (l2net *l2)
{
    std::cout << sizeof *l2 << "\n" ;	// calm down -Wall
}

/******************************************************************************
 * Add a new slave:
 * - initialize the status
 * - add it to the slave list
 * - (no need to notify the sender thread)
 */

void sos::add_slave (slave *s)
{
    std::lock_guard <std::mutex> lk (mtx_) ;

    s->reset () ;
    slist_.push_front (*s) ;
}

/******************************************************************************
 * Add a new message to send
 */

void sos::add_request (msg *m)
{
    std::lock_guard <std::mutex> lk (mtx_) ;

    mlist_.push_front (m) ;
    condvar_.notify_one () ;
}

/******************************************************************************
 * Sender thread
 * Block on a condition variable, waiting for events:
 * - timer event, signalling that a request timeout has expired
 * - change in L2 network handling (new L2, or removed L2)
 * - a new request is added (from an exterior thread)
 */

void sos::sender_thread (void)
{
    std::unique_lock <std::mutex> lk (mtx_) ;
    timepoint_t now, next_timeout ;

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

	    assert (std::chrono::duration_cast<duration_t> (delay).count() >= 0) ;

	    D ("WAIT " << std::chrono::duration_cast<duration_t> (delay).count() << "ms") ;
	    condvar_.wait_for (lk, delay) ;
	}

	/*
	 * we have been awaken for (one or more) multiple reasons:
	 * - a new l2 network has been registered
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
		ri->thr = new std::thread (&sos::receiver_thread, this, &*ri) ;
	    }

	    // is it time to send a new hello ?
	    if (now >= ri->next_hello)
	    {
		// send the pre-prepared hello message
		ri->hellomsg->id (0) ;		// don't reuse the same msg id
		ri->hellomsg->send () ;
		// schedule next hello packet
		ri->next_hello = now + duration_t (INTERVAL_HELLO) ;
	    }
	}

	/*
	 * Traverse slave list to check ttl
	 */

	for (auto &s : slist_)
	{
	    // must current timeout be the next hello?
	    if (s.status () == slave::SL_RUNNING && now >= s.next_timeout_)
		s.reset () ;
	}

	/*
	 * Traverse message list to check new messages to send or older
	 * messages to retransmit
	 */

	mi = mlist_.begin () ;
	while (mi != mlist_.end ())
	{
	    msg *m ;
	    auto newmi = mi ;		// backup mi since we may erase it
	    newmi++ ;

	    /*
	     * New message to transmit or old message to retransmit
	     */

	    m = *mi ;
	    if (m->ntrans_ == 0 ||
		    (m->ntrans_ < MAX_RETRANSMIT && now >= m->next_timeout_)
		    )
	    {
		if (m->send () == -1)
		{
		    std::cout << "ERROR DURING TRANSMISSION\n" ;
		}
	    }

	    /*
	     * Message expired
	     */

	    if (now >= m->expire_)
	    {
		D ("ERASE id=" << m->id ()) ;
		delete m ;
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

	for (auto &s : slist_)
	{
	    // must current timeout be the next hello?
	    if (next_timeout > s.next_timeout_)
		next_timeout = s.next_timeout_ ;
	}

	for (auto &m : mlist_)
	{
	    // must current timeout be the next retransmission of this message?
	    if (m->ntrans_ < MAX_RETRANSMIT && next_timeout > m->next_timeout_)
		next_timeout = m->next_timeout_ ;
	}
    }
}

/******************************************************************************
 * Receiver thread
 * Block on packet reception on the given interface
 */

bool sos::find_peer (msg *m, l2addr *a, receiver *r)
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
	    if (s.addr () != 0 && *a == *(s.addr ()))
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

	    sid = m->is_sos_discover () ;
	    if (sid)
	    {
		for (auto &s : slist_)
		{
		    if (sid == s.slaveid ())
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

/*
 * Message correlation: see CoAP spec, section 4.4
 * Is the received message a reply to an already sent request?
 * For this, we need to perform a search in the message list
 * for a request message with this id.
 */

msg *sos::correlate (msg *m)
{
    msg::msgtype mt ;
    msg *orgmsg ;

    orgmsg = 0 ;
    mt = m->type () ;
    if (mt == msg::MT_ACK || mt == msg::MT_RST)
    {
	std::lock_guard <std::mutex> lk (mtx_) ;
	int id ;

	id = m->id () ;
	for (auto &mr : mlist_)
	{
	    if (mr->id () == id)
	    {
		/* Got it! Original request found */
		D ("Found original request for id=" << id) ;
		orgmsg = &*mr ;
		break ;
	    }
	}
    }

    return orgmsg ;
}

void sos::clean_deduplist (receiver *r)
{
    std::list <msg *>::iterator di ;
    timepoint_t now ;

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

/*
 * Check if a message is a duplicate
 * Returns the original message if found.
 * If an answer has already been sent (the message is marked as such with
 * the reqrep method), send it back again
 */

msg *sos::deduplicate (receiver *r, msg *m)
{
    msg::msgtype mt ;
    msg *orgmsg ;

    orgmsg = 0 ;			// no duplicate
    mt = m->type () ;
    if (mt == msg::MT_CON || mt == msg::MT_NON)
    {
	for (auto &d : r->deduplist)
	{
	    if (*m == *d)
	    {
		orgmsg = *(&d) ;
		break ;
	    }
	}

	if (orgmsg && orgmsg->reqrep ())
	{
	    /*
	     * Found a duplicated message (and an answer). Just send
	     * back the already sent answer.
	     */
	    D ("DUPLICATE MESSAGE id=" << orgmsg->id ()) ;
	    (void) orgmsg->reqrep ()->send () ;
	}
	else
	{
	    // store the new message on deduplist with a timeout
	    m->expire_ = DATE_TIMEOUT_MS (EXCHANGE_LIFETIME (r->l2->maxlatency ())) ;
	    r->deduplist.push_front (m) ;
	}
    }

    return orgmsg ;
}

void sos::receiver_thread (receiver *r)
{
    for (;;)
    {
	msg *m ;			// received message
	l2addr *a ;			// source address of received message
	msg *orgreq ;			// message correlation result
	msg *dupmsg ;			// original message in case of duplicate

	/*
	 * Wait for a new message
	 */

	m = new msg ;
	a = m->recv (r->l2) ;

	/*
	 * House cleaning: remove obsolete messages from deduplication list
	 */

	clean_deduplist (r) ;

	/*************************************************************
	 * CoAP message pre-processing (deduplication, correlation, etc.)
	 */

	/*
	 * Find slave. If not found, the message is ignored
	 */

	if (! find_peer (m, a, r))
	{
		D("\033[31m ! find_peer\033[00m");
	    continue ;
	}

	/*
	 * Is the received message a reply to a pending request?
	 */

	orgreq = correlate (m) ;
	if (orgreq)
	{
	    /*
	     * Ignore the message if an answer has already been received
	     */

	    if (orgreq->reqrep ())
		continue ;

	    /*
	     * This is the first reply we get.
	     * Stop further retransmissions and link the received answer to
	     * the original request we sent.
	     */

	    orgreq->link_reqrep (m) ;
	    orgreq->stop_retransmit () ;
	}

	/*
	 * Is this the same request as already seen?
	 * Ignore it if an answer has already been sent (in which
	 * case the deduplicate function send it back again).
	 */

	dupmsg = deduplicate (r, m) ;
	if (dupmsg && dupmsg->reqrep ())
	    continue ;

	/*************************************************************
	 * Message processing
	 */

	/*
	 * Check SOS control messages first
	 */

	if (m->sos_type () != msg::SOS_NONE)
	{
	    m->peer ()->process_sos (this, m) ;
	    continue ;
	}

	if (m->peer ()->status () == slave::SL_RUNNING)
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

}					// end of namespace sos
