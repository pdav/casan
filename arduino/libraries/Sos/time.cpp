#include "time.h"

#define	TIME_HIGH(t)		((uint32_t) (((t) >> 32) & 0xffffffff))
#define	TIME_LOW(t)		((uint32_t) (((t)      ) & 0xffffffff))
#define	MK_TIME(n,ms)		((((uint64_t) (n)) << 32) | ms)

time current_time ;

time::time ()
{
    time_ = 0 ;
}

time::time (time &t)
{
    time_ = t.time_ ;
}

time::~time ()
{
}

bool time::operator< (time &t)
{
    return time_ < t.time_ ;
}

time &time::operator= (time &t)
{
    if (this != &t)
	memcpy (this, &t, sizeof *this) ;
    return *this ;
}

// update current time with help of millis ()
void time::cur (void)		
{
    uint32_t ms ;
    uint32_t n ;

    n = TIME_HIGH (time_) ;
    ms = millis () ;			// current time
    if (ms < TIME_LOW (time_))		// rollover?
	n++ ;
    time_ = MK_TIME (n, ms) ;
}

void time::add (unsigned long int time)
{
    time_ += (uint64_t) time ;
}

void time::add (time &t)
{
    time_ += t.time_ ;
}

timediff_t time::diff (time &a, time &b)
{
    timediff_t diff ;

    diff = a.time_ - b.time_ ;
    return diff ;
}

void time::print ()
{
    Serial.print (F ("\033[33mtime = \033[00m")) ;
    Serial.print (TIME_HIGH (time_)) ;
    Serial.print (F ("\033[33m,\033[00m")) ;
    Serial.print (TIME_LOW (time_)) ;
    Serial.println () ;
}
