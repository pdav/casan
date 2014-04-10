#include "time.h"

#define	TIME_HIGH(t)		((uint32_t) (((t) >> 32) & 0xffffffff))
#define	TIME_LOW(t)		((uint32_t) (((t)      ) & 0xffffffff))
#define	MK_TIME(n,ms)		((((uint64_t) (n)) << 32) | ms)

time curtime ;

// synchronize current time with help of millis ()
void time::sync (void)		
{
    millis_t ms ;
    uint32_t n ;

    n = TIME_HIGH (time_) ;
    ms = millis () ;			// current time
    if (ms < TIME_LOW (time_))		// rollover?
	n++ ;
    time_ = MK_TIME (n, ms) ;
}

void time::print ()
{
    Serial.print (F ("\033[33mtime = \033[00m")) ;
    Serial.print (TIME_HIGH (time_)) ;
    Serial.print (F ("\033[33m:\033[00m")) ;
    Serial.print (TIME_LOW (time_)) ;
    Serial.println () ;
}
