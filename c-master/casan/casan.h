/**
 * @file casan.h
 * @brief CASAN engine interface
 */

#ifndef CASAN_CASAN_H
#define	CASAN_CASAN_H

#include <list>
#include <string>
#include <memory>

#include <thread>
#include <mutex>
#include <condition_variable>

#include "msg.h"
#include "slave.h"

namespace casan {

class l2net ;

/**
 * @brief CASAN engine class
 *
 * This class does all the hard work of CASAN master. It manages and
 * implements all CASAN threads (see below), requests, timers and
 * retransmissions.
 *
 * There is one sender thread in the CASAN system: it processes send
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

class casan
{
    public:
	casan () ;
	~casan () ;

	friend std::ostream& operator<< (std::ostream &os, const casan &se) ;

	// start sender thread
	void init (void) ;

	// accessors and mutators
	casantimer_t timer_slave_ttl (void)	{ return slave_ttl_ ; }
	casantimer_t timer_first_hello (void)	{ return first_hello_ ; }
	casantimer_t timer_interval_hello (void) { return interval_hello_ ; }
	void timer_slave_ttl (casantimer_t t) 	{ slave_ttl_ = t ; }
	void timer_first_hello (casantimer_t t) { first_hello_ = t ; }
	void timer_interval_hello (casantimer_t t) { interval_hello_ = t ; }

	// start and stop receiver thread
	void start_net (l2net *l2) ;
	void stop_net (l2net *l2) ;

	// add a known slave (may be off)
	void add_slave (slave *s) ;

	// add a request
	void add_request (msgptr_t m) ;

	// find a slave by it's slave id
	slave *find_slave (slaveid_t sid) ;

	// dump data structures
	std::string html_debug (void) ;
	std::string resource_list (void) ;	// aggregated .well-known/casan

    private:
	struct receiver ;		// receiver private data

	std::list <receiver *> rlist_ ;	// connected networks
	std::list <slave> slist_ ;	// registered slaves
	std::list <msgptr_t> mlist_ ;	// messages sent by CASAN

	std::thread *tsender_ ;

	std::mutex mtx_ ;
	std::condition_variable condvar_ ;

	casantimer_t first_hello_ ;	// delay before first hello message
	casantimer_t interval_hello_ ;	// hello message interval
	casantimer_t slave_ttl_ ;	// default slave ttl (in sec)

	void sender_thread (void) ;
	void receiver_thread (receiver *r) ;
	void clean_deduplist (receiver &r) ;
	msgptr_t deduplicate (receiver &r, msgptr_t m) ;
	bool find_peer (msgptr_t m, l2addr *a, receiver &r) ;
	msgptr_t correlate (msgptr_t m) ;
} ;

}					// end of namespace casan
#endif
