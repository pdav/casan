#ifndef SOS_ENGINE_H
#define	SOS_ENGINE_H

#include <list>

#include <thread>
#include <mutex>
#include <condition_variable>

#include "l2.h"
#include "req.h"

typedef struct receiver *receiver_t ;
typedef struct client   *client_t ;

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

	// add a known client (may be off)
	void add_client (l2net *l2, l2addr *a, req_handler_t handler) ;

	// add a request
	void add_request (request &r) ;

    private:
	receiver_t rlist ;
	client_t clist ;

	std::list <request> qlist ;

	std::thread *tsender ;
	std::mutex mut ;
	std::condition_variable condvar ;

	void sender (void) ;
	void receiver (l2net *l2) ;
} ;

#endif
