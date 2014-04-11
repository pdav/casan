#ifndef __TIME_H__
#define __TIME_H__

/*
 * Since time counter on Arduino (see millis() function) rolls
 * back to 0 after 50 days, we must provide a longer time scale
 * in order to handle case where we are near to the limit (such
 * as after > before, always...).
 *
 * This file include various SOS timers definitions in order
 * hide implementation details. Note on these timers: they just
 * keep timepoints such that, when called, they can tell if
 * a given timer has expired. They don't keep an active time
 * measure, nor they don't provide callbacks.
 */

#include "Arduino.h"
#include "defs.h"

typedef uint64_t timediff_t ;
typedef uint64_t time_t ;

extern time_t curtime ;

/*
 * Current time
 *
 * To keep current time (for a longer scale than ~50 days), declare
 * a time_t variable:
 *	time_t curtime ;
 * and update it regularly (i.e. in the Arduino loop function) with:
 *	sync_time (curtime) ;
 */

extern void sync_time (time_t &cur) ;
extern void print_time (time_t &t) ;

/*
 * Timer used in waiting_unknown and waiting_known states
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

/*
 * Timer used in running and renew states
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
