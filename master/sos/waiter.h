#ifndef SOS_WAITER_H
#define SOS_WAITER_H

/*
 * Wait class
 */

namespace sos {

class waiter
{
    public:
	typedef std::function <void (void)> action_t ;

	void do_and_wait (action_t a) ;
	void do_and_wait (action_t a, timepoint_t max) ;
	void wakeup (void) ;

    private:
	std::mutex mtx_ ;
	std::condition_variable condvar_ ;
} ;

}					// end of namespace sos
#endif
