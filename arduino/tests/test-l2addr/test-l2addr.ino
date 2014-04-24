/*
 * Test program for the "l2addr_*" class
 */

#include "sos.h"

#ifdef L2_ETH
    #include "l2-eth.h"
#endif
#ifdef L2_154
    #include "l2-154.h"
#endif

#define	DEBUGINTERVAL	2

Debug debug ;

void setup ()
{
    Serial.begin (38400) ;
    debug.start (DEBUGINTERVAL) ;
}

void test_l2addr (void)
{
#ifdef L2_ETH
    l2addr_eth *x = new l2addr_eth ("0a:0b:0c:0a:0b:0c") ;
    l2addr_eth *y = new l2addr_eth ("00:00:00:00:00:00") ;
#endif
#ifdef L2_154
    l2addr_154 *x = new l2addr_154 ("ca:fe") ;
    l2addr_154 *y = new l2addr_154 ("be:ef") ;
#endif

    if (*x == *y)
    {
	x->print () ; Serial.println () ;
	y->print () ; Serial.println () ;
	PRINT_DEBUG_STATIC ("x == y (PROBLEM)") ;
    }
    else
    {
	x->print () ; Serial.println () ;
	y->print () ; Serial.println () ;
	PRINT_DEBUG_STATIC ("x != y (OK)") ;
    }
    *x = *y ;
    if (*x == *y)
    {
	x->print () ; Serial.println () ;
	y->print () ; Serial.println () ;
	PRINT_DEBUG_STATIC ("x == y (OK)") ;
    }
    else
    {
	x->print () ; Serial.println () ;
	y->print () ; Serial.println () ;
	PRINT_DEBUG_STATIC ("x != y (BAD)") ;
    }

    delete x ;
    delete y ;
}

void loop () 
{
    if (debug.heartbeat ())
	test_l2addr () ;
}
