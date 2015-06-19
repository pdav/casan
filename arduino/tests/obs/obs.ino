/*
 * Example program for observing CASAN resources
 *
 * 2 resources are declared:
 * - v1 is observable
 * - v2 is not observable
 */

#include "casan.h"

#ifdef L2_ETH
    #include "l2-eth.h"

    l2addr *myaddr = new l2addr_eth ("00:01:02:03:04:05") ;
    l2net_eth l2 ;
    // MTU is less than 0.25 * (free memory in SRAM after initialization)
    #define	MTU		200
#endif
#ifdef L2_154
    #include "l2-154.h"

    l2addr *myaddr = new l2addr_154 ("45:67") ;
    l2net_154 l2 ;
    // #define	CHANNEL		25
    #define	CHANNEL		26
    #define	PANID		CONST16 (0xca, 0xfe)
    #define	MTU		0
#endif

#define	DEBUGINTERVAL	10
#define	SLAVEID		170

int tmp_sensor = A0 ;
int led = A2 ;

Casan *casan ;
Debug debug ;

unsigned long int request_sensor (void)
{
    // simulate a sensor with a value which changes every 10 seconds
    return millis () / (20 * 1000) ;
}

unsigned long int oldval ;		/* global variable, for observation */

uint8_t process_request (Msg *in, Msg *out) 
{
    char payload [10] ;

    // out->max_age (true, 0) ;		/* answer not cachable */

    DBGLN1 (F ("process_request")) ;

    unsigned long int newval = request_sensor () ;
    snprintf (payload, 10, "%lu", newval) ;

    out->set_payload ((uint8_t *) payload,  strlen (payload)) ;

    oldval = newval ;

    return COAP_RETURN_CODE (2, 5) ;
}

void register_observer (Msg &in)
{
    DBG1 (F ("Register observer ")) ;
    DBGLN0 () ;
}

void deregister_observer (void)
{
    DBG1 (F ("Deregister observer ")) ;
    DBGLN0 () ;
}

int sensor_trigger (void)
{
    unsigned long int newval = request_sensor () ;

    return oldval != newval ;
}

void setup () 
{
    pinMode (tmp_sensor, INPUT) ;     
    pinMode (led, OUTPUT) ;     

    Serial.begin (38400) ;
#ifdef L2_ETH
    l2.start (myaddr, false, ETH_TYPE) ;
#endif
#ifdef L2_154
    l2.start (myaddr, false, CHANNEL, PANID) ;
#endif
    casan = new Casan (&l2, MTU, SLAVEID) ;

    debug.start (DEBUGINTERVAL) ;

    /* definitions for a resource: name (in URL), title, rt for /.well-known */

    Resource *r1 = new Resource ("v1", "Sensor", "sec") ;
    r1->handler (COAP_CODE_GET, process_request) ;
    r1->ohandler (register_observer, deregister_observer, sensor_trigger) ;
    casan->register_resource (r1) ;

    Resource *r2 = new Resource ("v2", "Sensor", "sec") ;
    r2->handler (COAP_CODE_GET, process_request) ;
    casan->register_resource (r2) ;

    casan->print_resources () ;
}

void loop () 
{
    debug.heartbeat () ;
    casan->loop () ;
}
