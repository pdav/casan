/**
 * @file time.h
 * @brief time related types and classes
 *
 * This file include various SOS timers definitions in order
 * hide implementation details.
 */

#ifndef __TIME_H__
#define __TIME_H__

#include <Arduino.h>
#include "defs.h"

/** @brief Type for current time value in milliseconds since last start
 * 
 * Since time counter on Arduino (see millis() function) rolls
 * back to 0 after 50 days, we must provide a longer time scale
 * in order to handle case where we are near to the limit (such
 * as after > before, always...).
 *
 */

typedef uint64_t time_t ;

/** @brief Type to represent time difference
 */

typedef uint64_t timediff_t ;

/** @brief Current time
 *
 * This variable is globally declared, such as every application
 * can make use of it.
 */

extern time_t curtime ;

/** @brief Synchronize current time
 *
 * This function synchronizes time in a variable with the help of
 * the `millis()` function (standard Arduino library).
 *
 * It is meant to be used with the `curtime` global variable, but can
 * be used with any variable of type `time_t` (provided that it is
 * correctly initialized).
 *
 * Note that time synchronization cannot work if calls to this function
 * are spaced with more than ~50 days. As such, this function should be
 * called often (in the `loop` function for example).
 */

extern void sync_time (time_t &cur) ;

/** @brief Print current time as "<high>:<low>"
 */

extern void print_time (time_t &t) ;

/**
 * @brief SOS timer used in waiting_unknown and waiting_known states
 *
 * This class abstracts parameters for the timer used to send
 * SOS Discover messages while the SOS engine is in waiting_unknown
 * or waiting_known state.
 *
 * Note: this timer only keeps timepoints such that, when called,
 * it can tell if the event should occur. It does not keep an
 * active time measure, nor it does not provide callbacks.
 */

class Twait
{
    public:
	void init (time_t &cur) ;
	bool next (time_t &cur) ;
	bool expired (time_t &cur) ;
    private:
	time_t next_ ;
	time_t inc_ ;
	time_t limit_ ;
} ;

/** @class Trenew
 * @brief SOS timer used in running and renew states
 *
 * This class abstracts parameters for the timer used to keep
 * association running.
 *
 * Note: this timer only keeps timepoints such that, when called,
 * it can tell if the event should occur. It does not keep an
 * active time measure, nor it does not provide callbacks.
 */

class Trenew
{
    public:
	void init (time_t &cur, time_t sttl) ;
	bool renew (time_t &cur) ;		// time to enter renew state
	bool next (time_t &cur) ;		// next discover
	bool expired (time_t &cur) ;		// time to enter waiting_known
    private:
	time_t mid_ ;
	time_t inc_ ;
	time_t next_ ;
	time_t limit_ ;
} ;

#endif
