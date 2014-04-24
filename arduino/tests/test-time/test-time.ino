#include "sos.h"

/*
 * Test program for the "time" class
 */

#define	DEBUGINTERVAL	10

Debug debug ;

void setup ()
{
    Serial.begin (38400) ;
    debug.start (DEBUGINTERVAL) ;
    sync_time (curtime) ;
}

void test_diff (void)
{
    time_t x = 0 ;
    timediff_t diff ;

    Serial.print (F ("x : ")) ;
    print_time (x) ;
    Serial.println () ;

    Serial.print (F ("curtime : ")) ;
    sync_time (curtime) ;
    print_time (curtime) ;
    Serial.println () ;

    Serial.print (F ("diff : ")) ;
    diff = x - curtime ;
    Serial.println ((long int) diff) ;

    if (curtime < x)
	Serial.println (F ("\033[31m ISSUE : curtime < x \033[00m ")) ;

    if (x < curtime)
	Serial.println (F ("\033[32m OK : x < curtime \033[00m ")) ;

}

void test_operators (void)
{
    time_t x, y ;
    timediff_t d ;

    Serial.print (F ("x : ")) ;
    print_time (x) ;

    Serial.println (F ("x = curtime ")) ;
    x = curtime ;

    Serial.print (F ("x : ")) ;
    print_time (x) ;
    Serial.println () ;

    Serial.print (F ("curtime : ")) ;
    print_time (curtime) ;
    Serial.println () ;

    Serial.println (F ("x + 5000 ; ")) ;
    x = x + 5000 ;
    Serial.print (F ("x : ")) ;
    print_time (x) ;
    Serial.println () ;

    Serial.println (F ("y = x ; y = y + 5000 ; ")) ;
    y = x ;
    y = y + 5000 ;
    Serial.print (F ("y : ")) ;
    print_time (y) ;
    Serial.println () ;
    Serial.print (F ("x : ")) ;
    print_time (x) ;
    Serial.println () ;

    Serial.println (F ("d = y - x : ")) ;
    d = y - x ;
    Serial.println ((unsigned long int) d) ;

    Serial.println (F ("y = y - 5000 ;")) ;
    y = y - 5000 ;
    print_time (y) ;
    Serial.println () ;

    sync_time (curtime) ;
}

void loop ()
{
    if (debug.heartbeat ())
    {
	unsigned long t = millis () ;
	PRINT_DEBUG_STATIC ("MILLIS : ") ;
	PRINT_DEBUG_DYNAMIC (t) ;
	test_diff () ;
	test_operators () ;
    }
}
