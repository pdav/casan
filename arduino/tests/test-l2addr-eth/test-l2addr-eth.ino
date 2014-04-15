/*
 * Test program for the "l2addr_eth" class
 */

#include <SPI.h>
#include "sos.h"

#define	DEBUGINTERVAL	1

void setup ()
{
    Serial.begin (38400) ;
    debug.start (DEBUGINTERVAL) ;
}

void test_l2addr (void)
{
    l2addr_eth *x = new l2addr_eth ("0a:0b:0c:0a:0b:0c") ;
    l2addr_eth *y = new l2addr_eth ("00:00:00:00:00:00") ;

    if (*x == *y)
    {
	x->print () ;
	y->print () ;
	PRINT_DEBUG_STATIC ("x == y (PROBLEM)") ;
    }
    else
    {
	x->print () ;
	y->print () ;
	PRINT_DEBUG_STATIC ("x != y (OK)") ;
    }
    *x = *y ;
    if (*x == *y)
    {
	x->print () ;
	y->print () ;
	PRINT_DEBUG_STATIC ("x == y (OK)") ;
    }
    else
    {
	x->print () ;
	y->print () ;
	PRINT_DEBUG_STATIC ("x != y (BAD)") ;
    }

    delete x ;
    delete y ;
}

void loop () 
{
    if (debug.heartbeat ())
    {
	PRINT_FREE_MEM ;
    }
    test_l2addr () ;
}
