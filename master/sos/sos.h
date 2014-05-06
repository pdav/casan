/**
 * @file sos.h
 * @brief SOS engine interface
 */

#ifndef SOS_SOS_H
#define	SOS_SOS_H

#include <list>
#include <string>
#include <memory>

#include <thread>
#include <mutex>
#include <condition_variable>

#include "slave.h"

namespace sos {

class msg ;
class slave ;
class l2net ;

/**
 * @brief SOS engine class
 *
 * This class does all the hard work of SOS master. It manages and
 * implements all SOS threads (see below), requests, timers and
 * retransmissions.
 *
 * There is one sender thread in the SOS system: it processes send
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

class sos
{
    public:
	sos () ;
	~sos () ;

	friend std::ostream& operator<< (std::ostream &os, const sos &se) ;

	// start sender thread
	void init (void) ;

	// accessors and mutators
	sostimer_t timer_slave_ttl (void)	{ return slave_ttl_ ; }
	sostimer_t timer_first_hello (void)	{ return first_hello_ ; }
	sostimer_t timer_interval_hello (void)	{ return interval_hello_ ; }
	void timer_slave_ttl (sostimer_t t) 	{ slave_ttl_ = t ; }
	void timer_first_hello (sostimer_t t) 	{ first_hello_ = t ; }
	void timer_interval_hello (sostimer_t t) { interval_hello_ = t ; }

	// start and stop receiver thread
	void start_net (l2net *l2) ;
	void stop_net (l2net *l2) ;

	// add a known slave (may be off)
	void add_slave (slave *s) ;

	// add a request
	void add_request (std::shared_ptr <msg> m) ;

	// find a slave by it's slave id
	slave *find_slave (slaveid_t sid) ;

	// dump data structures
	std::string html_debug (void) ;
	std::string resource_list (void) ;	// aggregated .well-known/sos

    private:
	struct receiver ;		// receiver private data

	std::list <receiver *> rlist_ ;	// connected networks
	std::list <slave> slist_ ;	// registered slaves
	std::list <std::shared_ptr <msg>> mlist_ ;	// messages sent by SOS

	std::thread *tsender_ ;

	std::mutex mtx_ ;
	std::condition_variable condvar_ ;

	sostimer_t first_hello_ ;	// delay before first hello message
	sostimer_t interval_hello_ ;	// hello message interval
	sostimer_t slave_ttl_ ;		// default slave ttl (in sec)

	void sender_thread (void) ;
	void receiver_thread (receiver *r) ;
	void clean_deduplist (receiver &r) ;
	std::shared_ptr <msg> deduplicate (receiver &r, std::shared_ptr <msg> m) ;
	bool find_peer (std::shared_ptr <msg> m, l2addr *a, receiver &r) ;
	std::shared_ptr <msg> correlate (std::shared_ptr <msg> m) ;
} ;

}					// end of namespace sos
#endif
