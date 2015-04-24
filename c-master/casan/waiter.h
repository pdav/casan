/**
 * @file waiter.h
 * @brief Waiter class interface
 */

#ifndef CASAN_WAITER_H
#define CASAN_WAITER_H

namespace casan {

/**
 * @brief Waiter class
 *
 * This class is a synchronisation point. It allows a thread to
 * perform an action and waits either for a time-out or for another
 * thread to wake it up.
 */

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

}					// end of namespace casan
#endif
