/*
 * Example program for SOS
 */

#include "sos.h"

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
    #define	CHANNEL		25
    #define	PANID		CONST16 (0xca, 0xfe)
    #define	MTU		0
#endif

#define	DEBUGINTERVAL	10

int tmp_sensor = A0 ;
int led = A2 ;

int slaveid = 169 ;

Sos *sos ;
Debug debug ;

uint8_t process_temp1 (Msg &in, Msg &out) 
{
    char payload [10] ;

    Serial.println (F ("process_temp")) ;

    int sensorValue = analogRead (tmp_sensor) ;
    snprintf (payload, 10, "%d", sensorValue) ;

    out.set_payload ((uint8_t *) payload,  strlen (payload)) ;

    return COAP_RETURN_CODE (2, 5) ;
}

uint8_t process_temp2 (Msg &in, Msg &out) 
{
    char payload [10] ;

    out.max_age (true, 60) ;		// answer is cacheable

    Serial.println (F ("process_temp")) ;

    int sensorValue = analogRead (tmp_sensor) ;
    snprintf (payload, 10, "%d", sensorValue) ;

    out.set_payload ((uint8_t *) payload,  strlen (payload)) ;

    return COAP_RETURN_CODE (2, 5) ;
}

uint8_t process_led (Msg &in, Msg &out) 
{
    Serial.println (F ("process_led")) ;

    int n ;
    char *payload = (char*) in.get_payload () ;
    if (payload != NULL && sscanf ((const char *) payload, "val=%d", &n) == 1)
    {
	analogWrite (led, n) ;
    }

    return COAP_RETURN_CODE (2,5) ;
}

void setup () 
{
    pinMode (tmp_sensor, INPUT) ;     
    pinMode (led, OUTPUT) ;     

    Serial.begin (38400) ;
#ifdef L2_ETH
    l2.start (myaddr, false, MTU, ETHTYPE) ;
#endif
#ifdef L2_154
    l2.start (myaddr, false, MTU, CHANNEL, PANID) ;
#endif
    sos = new Sos (&l2, slaveid) ;

    debug.start (DEBUGINTERVAL) ;

    /* definitions for a resource: name (in URL), title, rt for /.well-known */

    Resource *r1 = new Resource ("temp", "Desk temperature", "celsius") ;
    r1->handler (COAP_CODE_GET, process_temp1) ;
    sos->register_resource (r1) ;

    Resource *r2 = new Resource ("temp", "Coffee room temperature", "celsius") ;
    r2->handler (COAP_CODE_GET, process_temp2) ;
    sos->register_resource (r2) ;

    Resource *r3 = new Resource ("led", "My beautiful LED", "light") ;
    r3->handler (COAP_CODE_GET, process_led) ;
    sos->register_resource (r3) ;

    sos->print_resources () ;
}

void loop () 
{
    debug.heartbeat () ;
    sos->loop () ;
}
