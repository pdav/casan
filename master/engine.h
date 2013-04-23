#ifndef SOS_ENGINE_H
#define	SOS_ENGINE_H

#include <list>

#include <thread>
#include <mutex>
#include <condition_variable>

#include "sos.h"
#include "l2.h"
#include "slave.h"
#include "req.h"

typedef struct receiver *receiver_t ;

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
	void add_slave (slave &s) ;

	// add a request
	void add_request (request &r) ;

    private:
	receiver_t rlist ;
	std::list <slave> slist ;
	std::list <request> qlist ;

	std::thread *tsender ;
	std::mutex mtx ;
	std::condition_variable condvar ;

	void sender (void) ;
	void receiver (l2net *l2) ;
} ;

#endif
