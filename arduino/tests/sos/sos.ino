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

#define	R1_name		"temp"
#define	R1_title	"temperature"
#define	R1_rt		"celcius"

#define	R2_name		"led"
#define	R2_title	"led"
#define	R2_rt		"light"

#define	DEBUGINTERVAL	10

int tmp_sensor = A0 ;
int led = A2 ;

int slaveid = 169 ;

Sos *sos ;

uint8_t process_temp (Msg &in, Msg &out) 
{
    Serial.println (F ("process_temp")) ;

    char message[10] ;
    for (int i (0) ; i < 10 ; i++)
	message[i] = '\0' ;

    int sensorValue = analogRead (tmp_sensor) ;
    snprintf (message, 10, "%d", sensorValue) ;

    out.set_payload ((uint8_t *) message,  strlen (message)) ;
    out.set_code (COAP_RETURN_CODE (2, 5)) ;

    return 0 ;
}

uint8_t process_led (Msg &in, Msg &out) 
{
    Serial.println (F ("process_led")) ;

    int n ;
    char * payload = (char*) in.get_payload () ;
    if (payload != NULL && sscanf ((const char *) payload, "val=%d", &n) == 1)
    {
	analogWrite (led, n) ;
    }

    out.set_code (COAP_RETURN_CODE (2,5)) ;

    return 0 ;
}

void setup () 
{
    pinMode (tmp_sensor, INPUT) ;     
    pinMode (led, OUTPUT) ;     

    Serial.begin (38400) ;
#ifdef L2_ETH
    l2.start (myaddr, false, MTU, SOS_ETH_TYPE) ;
#endif
#ifdef L2_154
    l2.start (myaddr, false, MTU, CHANNEL, PANID) ;
#endif
    sos = new Sos (&l2, slaveid) ;

    debug.start (DEBUGINTERVAL) ;

    Resource *r1 = new Resource (R1_name, R1_title, R1_rt) ;
    r1->handler (COAP_CODE_GET, process_temp) ;
    sos->register_resource (r1) ;

    Resource *r2 = new Resource (R2_name, R2_title, R2_rt) ;
    r2->handler (COAP_CODE_GET, process_led) ;
    sos->register_resource (r2) ;

    sos->print_resources () ;
}

void test_values_temp (void)
{
    Msg in, out ;
    process_temp (in, out) ;
    out.print () ;
}

void test_values_led (void)
{
    char h[10] ;
    char l[10] ;

    snprintf (h, 10, "%d", 1024) ;
    snprintf (l, 10, "%d", 0) ;

    Serial.println (h) ;
    Serial.println (l) ;

    option opt_high (option::MO_Uri_Query, (unsigned char*) h, strlen (h)) ;
    option opt_low (option::MO_Uri_Query, (unsigned char*) l, strlen (l)) ;

    Msg in, out ;
    Msg in2 ;

    in.push_option (opt_high) ;
    process_led (in, out) ;

    delay (500) ;
    in2.push_option (opt_low) ;
    process_led (in2, out) ;
}

void test_values (void)
{
    test_values_temp () ;
    test_values_led () ;
    delay (500) ;
}

void loop () 
{
    debug.heartbeat () ;

    //test_values () ;
    sos->loop () ;
}
