#ifndef SOS_SOS_H
#define	SOS_SOS_H

#include <list>
#include <string>

#include <thread>
#include <mutex>
#include <condition_variable>

#include "slave.h"

namespace sos {

class msg ;
class slave ;
class l2net ;

struct receiver ;

class sos
{
    public:
	sos () ;
	~sos () ;

	friend std::ostream& operator<< (std::ostream &os, const sos &se) ;

	// start sender thread
	void init (void) ;

	// accessors and mutators
	sostimer_t timer_slave_ttl (void) ;
	sostimer_t timer_first_hello (void) ;
	sostimer_t timer_interval_hello (void) ;
	void timer_slave_ttl (sostimer_t t) ;
	void timer_first_hello (sostimer_t t) ;
	void timer_interval_hello (sostimer_t t) ;

	// start and stop receiver thread
	void start_net (l2net *l2) ;
	void stop_net (l2net *l2) ;

	// add a known slave (may be off)
	void add_slave (slave *s) ;

	// add a request
	void add_request (msg *m) ;

	// find a slave by it's slave id
	slave *find_slave (slaveid_t sid) ;

	// dump data structures
	std::string html_debug (void) ;
	std::string resource_list (void) ;	// aggregated .well-known/sos

    private:
	std::list <receiver> rlist_ ;	// connected networks
	std::list <slave> slist_ ;	// registered slaves
	std::list <msg *> mlist_ ;	// messages sent by SOS

	std::thread *tsender_ ;
	std::mutex mtx_ ;
	std::condition_variable condvar_ ;

	sostimer_t first_hello_ ;	// delay before first hello message
	sostimer_t interval_hello_ ;	// hello message interval
	sostimer_t slave_ttl_ ;		// default slave ttl (in sec)

	void sender_thread (void) ;
	void receiver_thread (receiver *r) ;
	void clean_deduplist (receiver &r) ;
	msg *deduplicate (receiver &r, msg *m) ;
	bool find_peer (msg *m, l2addr *a, receiver &r) ;
	msg *correlate (msg *m) ;
} ;

}					// end of namespace sos
#endif
