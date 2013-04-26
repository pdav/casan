#ifndef SOS_ENGINE_H
#define	SOS_ENGINE_H

#include <list>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

#include "sos.h"
#include "l2.h"
#include "slave.h"
#include "msg.h"

struct receiver ;

class engine
{
    public:
	engine () ;
	~engine () ;

	// start sender thread
	void init (void) ;

	// start and stop receiver thread
	void start_net (l2net *l2) ;
	void stop_net (l2net *l2) ;

	// add a known slave (may be off)
	void add_slave (slave *s) ;

	// add a request
	void add_request (msg *m) ;

    private:
	std::list <receiver> rlist_ ;
	std::list <slave> slist_ ;
	std::list <msg> mlist_ ;

	std::thread *tsender_ ;
	std::mutex mtx_ ;
	std::condition_variable condvar_ ;

	void sender_thread (void) ;
	void receiver_thread (receiver *r) ;
} ;

#endif
