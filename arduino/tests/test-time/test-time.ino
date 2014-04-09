#include "sos.h"

/*
 * Test program for the "time" class
 */

// implicitely declared
// time current_time ;

void setup () {
    Serial.begin(38400) ;		// no space before left parenthesis
    Serial.println (F ("start")) ;
    current_time.sync () ;
}

void test_diff (void)
{
    time x ;
    timediff_t diff ;

    Serial.print (F ("x (unitialized) : ")) ;
    x.print () ;

    Serial.print (F ("current_time : ")) ;
    current_time.sync () ;
    current_time.print () ;

    Serial.print (F ("diff : ")) ;
    diff = x - current_time ;
    Serial.println ((long int) diff) ;

    if (current_time < x)
	Serial.println (F ("\033[31m ISSUE : current_time < x \033[00m ")) ;

    if (x < current_time)
	Serial.println (F ("\033[32m OK : x < current_time \033[00m ")) ;

}

void test_operators (void)
{
    time x, y ;
    timediff_t d ;

    Serial.print (F ("x : ")) ;
    x.print () ;

    Serial.println (F ("x = current_time ")) ;
    x = current_time ;

    Serial.print (F ("x : ")) ;
    x.print () ;

    Serial.print (F ("current_time : ")) ;
    current_time.print () ;

    Serial.println (F ("x + 5000 ; ")) ;
    x = x + 5000 ;
    Serial.print (F ("x : ")) ;
    x.print () ;

    Serial.println (F ("y = x ; y = y + 5000 ; ")) ;
    y = x ;
    y = y + 5000 ;
    Serial.print (F ("y : ")) ;
    y.print () ;
    Serial.print (F ("x : ")) ;
    x.print () ;

    Serial.println (F ("d = y - x : ")) ;
    d = y - x ;
    Serial.println ((unsigned long int) d) ;

    Serial.println (F ("y = y - 5000 ;")) ;
    y = y - 5000 ;
    y.print () ;

    current_time.sync () ;
}

void loop () {
    PRINT_DEBUG_STATIC ("\033[36m\tloop \033[00m ") ;

    // to check if we have memory leak
    PRINT_FREE_MEM ;

    unsigned long t = millis () ;
    PRINT_DEBUG_STATIC ("MILLIS : ") ;
    PRINT_DEBUG_DYNAMIC (t) ;
    test_diff () ;
    test_operators () ;

    delay (10000) ;
}
