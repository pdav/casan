/**
 * @file waiter.cc
 * @brief Waiter class implementation
 */

#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "global.h"

#include "waiter.h"

namespace sos {

/**
 * @brief Perform an action and wait indefinetely for an explicit wake-up
 *
 * @param a function pointer
 */

void waiter::do_and_wait (action_t a)
{
    do_and_wait (a, std::chrono::system_clock::time_point::max ()) ;
}

/**
 * @brief Perform an action and wait for a time-out or an explicit wake-up
 *
 * This methods calls the function (specified by the pointer, without
 * any argument) and waits for:
 * - either another thread wakes us up by calling waiter::wakeup
 * - or the time-out expires
 *
 * @param a function pointer
 * @param max maximum point in time where this function must return
 */

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

/**
 * @brief Wake up a thread waiting with waiter::do_and_wait
 */

void waiter::wakeup (void)
{
    condvar_.notify_all () ;
}

}				// end of sos namespace
