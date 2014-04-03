#ifndef __TIME_H__
#define __TIME_H__

/*
 * Since time counter on Arduino (see millis() function) rolls
 * back to 0 after 50 days, we must provide a longer time scale
 * in order to handle case where we are near to the limit (such
 * as after > before, always...).
 *
 * XXX : rework this class to use 64 bits arithmetic
 */

#include "Arduino.h"
#include "defs.h"

typedef int64_t timediff_t ;

class time
{
    public:
	time () ;
	time (time &t) ;
	~time () ;

	bool operator< (time &t) ;
	time &operator= (time &t) ;

	void cur (void) ;		// get current time
	void add (time &t) ;
	void add (unsigned long time) ;

	static timediff_t diff (time &a, time &b) ;

	void print () ;

    private:
	uint64_t time_ ;		// high:nb of rollover, low:millis
} ;

extern time current_time ;

#endif
