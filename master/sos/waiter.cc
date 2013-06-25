#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "global.h"

#include "waiter.h"

namespace sos {

void waiter::do_and_wait (action_t a)
{
    do_and_wait (a, std::chrono::system_clock::time_point::max ()) ;
}

void waiter::do_and_wait (action_t a, timepoint_t max)
{
    std::unique_lock <std::mutex> lk (mtx_) ;

    // (*a) () ;
    a () ;

    if (max == std::chrono::system_clock::time_point::max ())
	condvar_.wait (lk) ;
    else
    {
	timepoint_t now = std::chrono::system_clock::now () ;
	auto delay = max - now ;	// needed precision for delay

	D ("WAIT " << std::chrono::duration_cast<duration_t> (delay).count() << "ms") ;
	condvar_.wait_for (lk, delay) ;
    }
}

void waiter::wakeup (void)
{
    condvar_.notify_all () ;
}

}				// end of sos namespace
