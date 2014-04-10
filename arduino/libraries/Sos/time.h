#ifndef __TIME_H__
#define __TIME_H__

/*
 * Since time counter on Arduino (see millis() function) rolls
 * back to 0 after 50 days, we must provide a longer time scale
 * in order to handle case where we are near to the limit (such
 * as after > before, always...).
 */

#include "Arduino.h"
#include "defs.h"

typedef int64_t timediff_t ;
typedef unsigned long int millis_t ;

class time
{
    public:
	time ()			{ time_ = 0 ; }
	time (const time &t)	{ time_ = t.time_ ; }
	~time ()		{}

	time &operator= (time t)	{ time_ = t.time_ ; return *this ; }
	time operator+= (millis_t t)	{ time_ += t ; return *this ; }

	bool operator<  (time &t)	{ return time_ <  t.time_ ; }
	bool operator<= (time &t)	{ return time_ <= t.time_ ; }
	bool operator>  (time &t)	{ return time_ >  t.time_ ; }
	bool operator>= (time &t)	{ return time_ >= t.time_ ; }

	void sync (void) ;		// synchronize with current time

	void print () ;

    private:
	uint64_t time_ ;		// high:nb of rollovers, low:millis

	friend time operator+ (time lhs, const time &rhs) ;
	friend time operator+ (time lhs, const timediff_t &rhs) ;
	friend timediff_t operator- (time lhs, const time &rhs) ;
	friend time operator- (time lhs, const millis_t &rhs) ;

} ;

inline time operator+ (time lhs, const time &rhs)
{
    lhs.time_ += rhs.time_ ;
    return lhs ;
}

inline time operator+ (time lhs, const timediff_t &rhs)
{
    lhs.time_ += rhs ;
    return lhs ;
}

inline timediff_t operator- (time lhs, const time &rhs)
{
    timediff_t diff ;

    diff = lhs.time_ - rhs.time_ ;
    return diff ;
}

inline time operator- (time lhs, const millis_t &rhs)
{
    lhs.time_ -= rhs ;
    return lhs ;
}

extern time curtime ;			// expected to be the current time

#endif
