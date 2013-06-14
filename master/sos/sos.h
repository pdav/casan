#ifndef SOS_SOS_H
#define	SOS_SOS_H

#include <list>
#include <string>

#include <thread>
#include <mutex>
#include <condition_variable>

#include "slave.h"

namespace sos {

typedef long int slavettl_t ;

class msg ;
class slave ;
class l2net ;

struct receiver ;

class sos
{
    public:
	sos () ;
	~sos () ;

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

	// dump data structures
	std::string html_debug (void) ;

    private:
	std::list <receiver> rlist_ ;	// connected networks
	std::list <slave> slist_ ;	// registered slaves
	std::list <msg *> mlist_ ;	// messages sent by SOS

	std::thread *tsender_ ;
	std::mutex mtx_ ;
	std::condition_variable condvar_ ;

	slavettl_t ttl_ ;		// default slave ttl (in sec)

	void sender_thread (void) ;
	void receiver_thread (receiver *r) ;
	void clean_deduplist (receiver *r) ;
	msg *deduplicate (receiver *r, msg *m) ;
	bool find_peer (msg *m, l2addr *a, receiver *r) ;
	msg *correlate (msg *m) ;
} ;

}					// end of namespace sos
#endif
