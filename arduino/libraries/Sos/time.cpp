/**
 * @file time.cpp
 * @brief implementation of time related classes
 */

#include "time.h"

/*
 * Timers values (expressed in ms)
 */

#define	TIMER_WAIT_START	1*1000		// initial between discover msg
#define	TIMER_WAIT_INC		1*1000
#define	TIMER_WAIT_INC_MAX	10*1000
#define	TIMER_WAIT_MAX		30*1000		// time in waiting_known

#define	TIMER_RENEW_MIN		500		// min time between discover


/*
 * Macros to handle 64 bits integers
 */

#define	TIME_HIGH(t)		((uint32_t) (((t) >> 32) & 0xffffffff))
#define	TIME_LOW(t)		((uint32_t) (((t)      ) & 0xffffffff))
#define	MK_TIME(n,ms)		((((uint64_t) (n)) << 32) | ms)

/*******************************************************************************
 * Current time
 */

time_t curtime ;			// global variable

// synchronize current time with help of millis ()
// Should be used on curtime global variable, but can also be used on any var
void sync_time (time_t &cur)		
{
    unsigned long int ms ;
    uint32_t n ;

    n = TIME_HIGH (cur) ;
    ms = millis () ;			// current time
    if (ms < TIME_LOW (cur))		// rollover?
	n++ ;
    cur = MK_TIME (n, ms) ;
}

void print_time (time_t &t)
{
    Serial.print (F ("\033[33mtime = \033[00m")) ;
    Serial.print (TIME_HIGH (t)) ;
    Serial.print (F ("\033[33m:\033[00m")) ;
    Serial.print (TIME_LOW (t)) ;
}

/*******************************************************************************
 * Timers
 */

/*
 * twait: to send Discover messages in waiting_unknown and waiting_known
 *      states
 */

/** @brief Initialize the timer with the current time
 */

void Twait::init (time_t &cur)
{
    limit_ = cur + TIMER_WAIT_MAX ;
    inc_ = TIMER_WAIT_START ;
    next_ = cur + inc_ ;
}

/** @brief Is it the time to send a new Discover message?
 */

bool Twait::next (time_t &cur)
{
    bool itstime = false ;

    if (cur >= next_)
    {
	itstime = true ;
	inc_ += TIMER_WAIT_INC ;
	if (inc_ > TIMER_WAIT_INC_MAX)
	    inc_ = TIMER_WAIT_INC_MAX ;
	next_ += inc_ ;
    }
    return itstime ;
}

/** @brief Is it the time for a transition from the waiting_known
 *	to the waiting_unknown state?
 */

bool Twait::expired (time_t &cur)
{
    return cur >= limit_ ;
}

/** @brief Initialize the timer with the current time and the Slave TTL
 *	returned by the master in its Assoc message.
 */

void Trenew::init (time_t &cur, time_t sttl)
{
    inc_ = sttl / 2 ;
    next_ = cur + inc_ ;
    limit_ = cur + sttl ;
}

/** @brief Is it the time to enter the renew state and begin to send
 *	Discover messages?
 */

bool Trenew::renew (time_t &cur)
{
    return next (cur) ;
}

/** @brief Is it the time to send a new Discover message?
 */

bool Trenew::next (time_t &cur)
{
    bool itstime = false ;

    if (cur >= next_)
    {
	itstime = true ;
	inc_ /= 2 ;
	if (inc_ <= TIMER_RENEW_MIN)
	    inc_ = TIMER_RENEW_MIN ;
	next_ += inc_ ;
    }
    return itstime ;
}

/** @brief Has association expired?
 */

bool Trenew::expired (time_t &cur)
{
    return cur >= limit_ ;
}
