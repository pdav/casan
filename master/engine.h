#ifndef SOS_ENGINE_H
#define	SOS_ENGINE_H

#include <list>

#include <thread>
#include <mutex>
#include <condition_variable>

#include "sos.h"
#include "l2.h"
#include "slave.h"
#include "msg.h"

typedef long int slavettl_t ;

struct receiver ;

class engine
{
    public:
	engine () ;
	~engine () ;

	// start sender thread
	void init (void) ;

	void ttl (slavettl_t t) ;
	slavettl_t ttl (void) ;

	// start and stop receiver thread
	void start_net (l2net *l2) ;
	void stop_net (l2net *l2) ;

	// add a known slave (may be off)
	void add_slave (slave *s) ;

	// add a request
	void add_request (msg *m) ;

    private:
	std::list <receiver> rlist_ ;	// connected networks
	std::list <slave> slist_ ;	// registered slaves
	std::list <msg *> mlist_ ;	// messages sent by SOS

	std::thread *tsender_ ;
	std::mutex mtx_ ;
	std::condition_variable condvar_ ;

	slavettl_t ttl_ ;		// default slave ttl

	void sender_thread (void) ;
	void receiver_thread (receiver *r) ;
	void clean_deduplist (receiver *r) ;
	msg *deduplicate (receiver *r, msg *m) ;
	bool find_peer (msg *m, l2addr *a, receiver *r) ;
	msg *correlate (msg *m) ;
} ;

#endif
