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
 *   a wake up of the emitting thread
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
#include <thread>
#include <mutex>
#include <condition_variable>

// don't define NDEBUG
#include <cassert>

#include "global.h"
#include "utils.h"

#include "l2.h"
#include "waiter.h"
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

/******************************************************************************
 * Engine dumper
 */

std::ostream& operator<< (std::ostream &os, const sos &se)
{
    os << "Receivers: " ;
    for (auto &r : se.rlist_)
    {
	os << "Recv hid=" << r.hid
	    << ", Deduplist (NON/CON received):"
	    << "\n" ;
	for (auto &m : r.deduplist)
	    os << *m ;
    }
    os << "\n" ;

    os << "Slaves:\n" ;
    for (auto &s : se.slist_)
	os << s ;

    os << "Messages (sent):\n" ;
    for (auto &m : se.mlist_)
	os << *m ;

    return os ;
}


/******************************************************************************
 * Accessors & mutators
 */

sostimer_t sos::timer_first_hello (void)
{
    return first_hello_ ;
}

sostimer_t sos::timer_interval_hello (void)
{
    return interval_hello_ ;
}

sostimer_t sos::timer_slave_ttl (void)
{
    return slave_ttl_ ;
}

void sos::timer_first_hello (sostimer_t t)
{
    first_hello_ = t ;
}

void sos::timer_interval_hello (sostimer_t t)
{
    interval_hello_ = t ;
}

void sos::timer_slave_ttl (sostimer_t t)
{
    slave_ttl_ = t ;
}

/******************************************************************************
 * Displays (in a string) dynamic data structures
 */

std::string sos::html_debug (void)
{
    std::ostringstream oss ;

    oss << "Delay for first HELLO message = " << first_hello_ << " s\n" ;
    oss << "HELLO interval = " << interval_hello_ << " s\n" ;
    oss << "Default TTL = " << slave_ttl_ << " s\n" ;
    oss << "Slaves:\n" ;
    for (auto &s : slist_)
	oss << s ;

    return oss.str () ;
}

/******************************************************************************
 * Returns aggregated .well-known/sos for all running slaves
 */

std::string sos::resource_list (void)
{
    std::string str = "" ;

    for (auto &s : slist_)
    {
	if (s.status_ == slave::SL_RUNNING)
	{
	    for (auto &r : s.resource_list ())
	    {
		std::ostringstream oss ;
		oss << r ;
		str += oss.str () ;
	    }
	}
    }
    return str ;
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

	r->next_hello = now + random_timeout (first_hello_ * 1000)  ;

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
 * Locate a slave by its slave id
 */

slave *sos::find_slave (slaveid_t sid)
{
    slave *r ;

    r = nullptr ;
    for (auto &s : slist_)
    {
	if (s.slaveid () == sid)
	{
	    r = &s ;
	    break ;
	}
    }
    return r ;
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
    timepoint_t now, next_timeout ;

    now = std::chrono::system_clock::now () ;
    next_timeout = std::chrono::system_clock::time_point::max () ;

    for (;;)
    {
	std::unique_lock <std::mutex> lk (mtx_) ;

	// D ("- SENDER THREAD ---------------------" << *this) ;

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

	for (auto &r : rlist_)
	{
	    // should the receiver thread be started?
	    if (r.thr == NULL)
	    {
		D ("Found a receiver to start") ;
		r.thr = new std::thread (&sos::receiver_thread, this, &r) ;
	    }

	    // is it time to send a new hello ?
	    if (now >= r.next_hello)
	    {
		// send the pre-prepared hello message
		r.hellomsg->id (0) ;		// don't reuse the same msg id
		r.hellomsg->send () ;
		// schedule next hello packet
		r.next_hello = now + duration_t (interval_hello_ * 1000) ;
	    }
	}

	/*
	 * Traverse slave list to check ttl
	 */

	for (auto &s : slist_)
	    if (s.status () == slave::SL_RUNNING && now >= s.next_timeout_)
		s.reset () ;

	/*
	 * Traverse message list to check new messages to send or older
	 * messages to retransmit
	 */

	auto mi = mlist_.begin () ;

	while (mi != mlist_.end ())
	{
	    msg *m = *mi ;

#if 0
	    D ("PARCOURS mlist_") ;
	    D ("m = " << m) ;
	    D (*m) ;
#endif

	    /*
	     * New message to transmit or old message to retransmit
	     */

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
		D ("ERASE FROM SENT id=" << m->id ()) ;

#if 0
		/*
		 * Unlink messages
		 */

		m->link_reqrep (nullptr) ;
#endif

		/*
		 * Process to removal
		 */

		delete m ;
		mi = mlist_.erase (mi) ;
	    }
	    else mi++ ;
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
	    if (s.status () == slave::SL_RUNNING && next_timeout > s.next_timeout_)
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

bool sos::find_peer (msg *m, l2addr *a, receiver &r)
{
    bool found ;

    found = false ;
    if (a != nullptr)
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
			s.l2 (r.l2) ;
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
	std::unique_lock <std::mutex> lk (mtx_) ;
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

void sos::clean_deduplist (receiver &r)
{
    timepoint_t now = std::chrono::system_clock::now () ;
    auto di = r.deduplist.begin () ;
    while (di != r.deduplist.end ())
    {
	msg *m = *di ;

#if 0
	D ("PARCOURS deduplist") ;
	D ("m = " << m) ;
	D ("*di = " << *di) ;
	D (*m) ;
#endif

	if (now >= m->expire_)
	{
	    D ("ERASE FROM DEDUP id=" << m->id ()) ;
	    delete m ;
	    di = r.deduplist.erase (di) ;
	}
	else di++ ;
    }
}

/*
 * Check if a message is a duplicate
 * Returns the original message if found.
 * If an answer has already been sent (the message is marked as such with
 * the reqrep method), send it back again
 */

msg *sos::deduplicate (receiver &r, msg *m)
{
    msg::msgtype mt ;
    msg *orgmsg ;

    orgmsg = 0 ;			// no duplicate
    mt = m->type () ;
    if (mt == msg::MT_CON || mt == msg::MT_NON)
    {
	for (auto &d : r.deduplist)
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
	    m->expire_ = DATE_TIMEOUT_MS (EXCHANGE_LIFETIME (r.l2->maxlatency ())) ;
	    r.deduplist.push_front (m) ;
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

	// D ("- RECEIVER THREAD -------------------\n" << *this) ;

	/*
	 * Wait for a new message
	 */

	m = new msg ;
	a = m->recv (r->l2) ;

	D ("Received a message from " << *a) ;

	/*
	 * Invalid message received
	 */

	if (a == nullptr)
	    continue ;

	m->expire_ = DATE_TIMEOUT_MS (EXCHANGE_LIFETIME (r->l2->maxlatency ())) ;

	/*
	 * House cleaning: remove obsolete messages from deduplication list
	 */

	clean_deduplist (*r) ;

	/*************************************************************
	 * CoAP message pre-processing (deduplication, correlation, etc.)
	 */

	/*
	 * Find slave. If not found, the message is ignored
	 * If found, a is either
	 */

	if (! find_peer (m, a, *r))
	{
	    D("Sender " << *a << " not found in authorized peers") ;
	    continue ;
	}

	/*
	 * Is the received message a reply to a pending request?
	 */

	orgreq = correlate (m) ;
	if (orgreq != nullptr)
	{
	    /*
	     * Ignore the message if an answer has already been received
	     */

	    if (orgreq->reqrep () != nullptr)
		continue ;

	    /*
	     * This is the first reply we get.
	     * Stop further retransmissions, link the received answer to
	     * the original request we sent, and wake the emitter up.
	     */

	    orgreq->link_reqrep (m) ;
	    orgreq->stop_retransmit () ;

	    if (orgreq->wt () != nullptr)
	    {
		orgreq->wt ()->wakeup () ;
		continue ;
	    }
	}

	/*
	 * Is this the same request as already seen?
	 * Ignore it if an answer has already been sent (in which
	 * case the deduplicate function send it back again).
	 */

	dupmsg = deduplicate (*r, m) ;
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

	/*
	 * If we get here, message is an orphaned message
	 */

	if (m->peer ()->status () == slave::SL_RUNNING)
	{
	    D ("Orphaned message from " << *(m->peer ()->addr ()) << ", id=" << m->id ()) ;
	}
    }
}

}					// end of namespace sos
