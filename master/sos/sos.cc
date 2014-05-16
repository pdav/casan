/**
 * @file sos.cc
 * @brief SOS engine implementation
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

/**
 * @brief Private data for each receiver thread
 */

struct sos::receiver
{
    l2net *l2 ;
    long int hid ; 			// hello id, initialized at start time
    slave broadcast ;
    std::list <msgptr_t> deduplist ;	// received messages
    msgptr_t hellomsg ;
    timepoint_t next_hello ;
    std::thread *thr ;
} ;


/**
 * @brief SOS engine constructor
 *
 * This constructor almost makes nothing: no thread is started until
 * the sos::init method is called.
 */

sos::sos ()
{
    tsender_ = NULL ;
}

/**
 * @brief SOS engine destructor
 *
 * This destructor removes all internal lists used by the SOS engine.
 *
 * @bug Threads should be cleanly removed.
 */

sos::~sos ()
{
    rlist_.clear () ;
    slist_.clear () ;
    mlist_.clear () ;
}

/**
 * @brief Start the SOS engine
 *
 * This function starts the SOS engine, i.e. the sender thread
 * with the sos::sender_thread function.
 * Note that each receiver thread will be started with an appropriate
 * call to sos::start_net.
 */

void sos::init (void)
{
    std::srand (std::time (0)) ;

    if (tsender_ == NULL)
    {
	tsender_ = new std::thread (&sos::sender_thread, this) ;
    }
}

/**
 * @brief Dumps the SOS engine data on the given stream
 *
 * This method is used for debugging purpose.
 *
 * @param os output stream
 * @param se SOS engine object
 */

std::ostream& operator<< (std::ostream &os, const sos &se)
{
    os << "Receivers: " ;
    for (auto &r : se.rlist_)
    {
	os << "Recv hid=" << r->hid
	    << ", Deduplist (NON/CON received):"
	    << "\n" ;
	for (auto &m : r->deduplist)
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

/**
 * @brief Dumps SOS engine configuration
 *
 * Returns a string with the SOS engine configuration (global
 * parameters as well as slave list) as a raw text (ready to
 * be enclosed in an HTML pre tag).
 *
 * @return string containing the output
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

/**
 * @brief Returns aggregated /.well-known/sos for all running slaves
 *
 * Returns a string containing the aggregated list (for all slaves) of
 * /.well-known/sos resource lists 
 *
 * @return string containing the global /.well-known/sos resource list
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

/**
 * @brief Add a new L2 network and start the receiver thread
 *
 * This methods adds a new L2 network to the list of networks
 * managed by the SOS engine, and asks the sender thread to
 * start a new receiver thread for this L2 network.
 *
 * In detail, this method:
 * - create a new receiver structure for receiver private data
 * - schedules the first HELLO packet
 * - adds the network to the receiver queue
 * - notify the sender thread in order to create a new receiver thread
 *
 * @param l2 pointer to an existing l2net object (which should be
 *	in reality a l2net_xxx object)
 */

void sos::start_net (l2net *l2)
{
    std::unique_lock <std::mutex> lk (mtx_) ;

    if (tsender_ != NULL)
    {
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

	r->hellomsg = std::make_shared <msg> () ;
	r->hellomsg->peer (& r->broadcast) ;
	r->hellomsg->type (msg::MT_NON) ;
	r->hellomsg->code (msg::MC_POST) ;
	r->hellomsg->mk_ctl_hello (r->hid) ;

	r->next_hello = now + random_timeout (first_hello_ * 1000)  ;

	rlist_.push_front (r) ;
	condvar_.notify_one () ;
    }
}

/**
 * @brief Remove an L2 network
 *
 * @bug This method should remove the associated thread and close
 *	the interface
 */

void sos::stop_net (l2net *l2)
{
    std::cout << sizeof *l2 << "\n" ;	// calm down -Wall
}

/**
 * @brief Add a new slave
 *
 * This method adds a new slave to the SOS engine. In detail,
 * this means:
 * - initialize the slave status
 * - add it to the slave list
 *
 * There is no need to notify the sender thread.
 */

void sos::add_slave (slave *s)
{
    std::unique_lock <std::mutex> lk (mtx_) ;

    s->reset () ;
    slist_.push_front (*s) ;
}

/**
 * @brief Locate a slave by its slave id
 *
 * This method searches through the slave list to find a slave
 * with its slave-id (as given by the slave in its Discover message).
 *
 * @param sid slave id
 * @return pointer to the found slave, or NULL if not found
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

/**
 * @brief Add a new message to send
 *
 * This method notifies the sender thread to send the given message
 * by pushing it in the retransmission list (list of outgoing messages).
 *
 * @param m pointer to the message to be sent
 */

void sos::add_request (msgptr_t m)
{
    std::unique_lock <std::mutex> lk (mtx_) ;

    mlist_.push_front (m) ;
    condvar_.notify_one () ;
}

/******************************************************************************
 * Sender thread
 *****************************************************************************/


/**
 * @brief Sender thread
 *
 * The sender thread spends its life blocking on a condition variable,
 * waiting for events:
 * - timer event, signalling that a request timeout has expired
 * - change in L2 network handling (new L2, or removed L2)
 * - a new request is added (from an exterior thread)
 *
 * The sender thread manages:
 * - the list of l2 networks (the thread must start a new receiver
 *     thread for each new l2 network)
 * - the list of all slaves in order to expire the "active" status
 *     if needed
 * - the list of all outgoing messages in order to:
 *    - send a new message
 *    - retransmit a message if no answer has been received yet
 *    - expire an old message without any received answer. In this
 *     case, the message will only be deleted if there is no
 *     thread waiting for this message.
 */

void sos::sender_thread (void)
{
    for (;;)
    {
	timepoint_t now ;
	timepoint_t next_timeout ;

	std::unique_lock <std::mutex> lk (mtx_) ;

	/*
	 * Sender thread is woken up (see last instructions in this loop
	 * body) for one or more multiple reasons:
	 * - a new l2 network has been registered
	 * - a new message is to be sent
	 * - timeout expired: there is an action to do (message to
	 *	retransmit or to remove from a queue)
	 * At each iteration, we need to check all reasons.
	 */

	now = std::chrono::system_clock::now () ;

	/*
	 * Traverse the receiver list to check new entries, and
	 * start receiver threads.
	 */

	for (auto &r : rlist_)
	{
	    // should the receiver thread be started?
	    if (r->thr == NULL)
	    {
		D ("Found a receiver to start") ;
		r->thr = new std::thread (&sos::receiver_thread, this, r) ;
	    }

	    // is it time to send a new hello ?
	    if (now >= r->next_hello)
	    {
		// send the pre-prepared hello message
		r->hellomsg->id (0) ;		// don't reuse the same msg id
		r->hellomsg->send () ;
		// schedule next hello packet
		r->next_hello = now + duration_t (interval_hello_ * 1000) ;
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

	for (auto &m : mlist_)
	{
	    if (m->ntrans_ == 0 ||
		    (m->ntrans_ < MAX_RETRANSMIT && now >= m->next_timeout_)
		    )
	    {
		if (m->send () == -1)
		{
		    std::cout << "ERROR DURING TRANSMISSION\n" ;
		}
	    }

	}

	/*
	 * Remove expired messages (using a C++ lambda)
	 */

	mlist_.remove_if (
	    [now]
	    (const msgptr_t &m)
	    {
		return now > m->expire_ ;
	    }) ;

	/*
	 * Determine date of next action
	 */

	next_timeout = std::chrono::system_clock::time_point::max () ;

	for (auto &r : rlist_)
	{
	    // must current timeout be the next hello?
	    if (next_timeout > r->next_hello)
		next_timeout = r->next_hello ;
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


	/*
	 * Wait for next action (or indefinitely)
	 */

	if (next_timeout == std::chrono::system_clock::time_point::max ())
	{
	    D ("WAIT") ;
	    condvar_.wait (lk) ;
	}
	else
	{
	    now = std::chrono::system_clock::now () ;
	    auto delay = next_timeout - now ;	// needed precision for delay

	    assert (std::chrono::duration_cast<duration_t> (delay).count() >= 0) ;

	    D ("WAIT " << std::chrono::duration_cast<duration_t> (delay).count() << "ms") ;
	    condvar_.wait_for (lk, delay) ;
	}
    }
}

/******************************************************************************
 * Receiver threads
 *****************************************************************************/

/**
 * @brief Receiver thread
 *
 * Each receiver thread spends its life waiting for a message to be
 * received on the associated interface (in fact, the l2net::recv
 * method).
 *
 * A receiver thread manages a de-duplication list (as specified
 * in the CoAP specification) to eliminate received answers which
 * are duplicated (i.e. already received).
 *
 * @param r receiver private data
 */

void sos::receiver_thread (receiver *r)
{
    for (;;)
    {
	msgptr_t m (new msg) ;	// received message
	l2addr *a ;			// source address of received message
	msgptr_t orgreq ;	// message correlation result
	msgptr_t dupmsg ;	// original message in case of duplicate

	// D ("- RECEIVER THREAD -------------------\n" << *this) ;

	/*
	 * Wait for a new message
	 */

	a = m->recv (r->l2) ;

	/*
	 * Invalid message received
	 */

	if (a == nullptr)
	    continue ;

	D ("Received a message from " << *a) ;
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
	    D("Sender not found in authorized peers") ;
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

	    msg::link_reqrep (orgreq, m) ;
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

bool sos::find_peer (msgptr_t m, l2addr *a, receiver &r)
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
	    int mtu ;

	    if (m->is_sos_discover (sid, mtu))
	    {
		for (auto &s : slist_)
		{
		    if (sid == s.slaveid ())
		    {
			int l2mtu, defmtu ;

			s.l2 (r.l2) ;
			s.addr (a) ; free_a = false ;
			m->peer (&s) ;

			// MTU negociation
			l2mtu = r.l2->mtu () ;	// MTU of network
			defmtu = s.defmtu () ;	// user configured network MTU
D ("l2mtu=" << l2mtu << ", defmtu=" << defmtu) ;
			if (defmtu <= 0 || defmtu > l2mtu)
			    defmtu = l2mtu ;
			if (mtu <= 0 || mtu > defmtu)
			    mtu = defmtu ;
D ("mtu=" << mtu) ;
			s.curmtu (mtu) ;

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

/**
 * @brief Message correlation
 *
 * Message correlation: see CoAP spec, section 4.4
 * Is the received message a reply to an already sent request?
 * For this, we need to perform a search in the message list
 * for a request message with this id.
 *
 * @param m received message
 * @return pointer to our original request if the message is
 *	is a reply, or NULL if the received message is not a
 *	a reply to a previous request
 */

msgptr_t sos::correlate (msgptr_t m)
{
    msg::msgtype mt ;
    msgptr_t orgmsg ;

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
		orgmsg = mr ;
		break ;
	    }
	}
    }

    return orgmsg ;
}

/**
 * @brief Remove obsolete items from deduplication list
 *
 * @param r receiver private data
 */

void sos::clean_deduplist (receiver &r)
{
    timepoint_t now = std::chrono::system_clock::now () ;

    /*
     * Remove expired messages (using a C++ lambda)
     */

    r.deduplist.remove_if (
	[now]
	(const msgptr_t &m)
	{
	    return now > m->expire_ ;
	}) ;

}

/**
 * @brief Check if a message is a duplicated one
 *
 * This method checks if a message has already been received.
 * If this is the case, returns the original message.
 * If an answer has already been sent (the message is marked as such with
 * the reqrep method), send it back again
 *
 * @param r receiver private data
 * @param m message to check
 * @return original message if m is a duplicate, or NULL if not found
 */

msgptr_t sos::deduplicate (receiver &r, msgptr_t m)
{
    msg::msgtype mt ;
    msgptr_t orgmsg ;

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
	}
    }
    r.deduplist.push_back (m) ;

    return orgmsg ;
}

}					// end of namespace sos
