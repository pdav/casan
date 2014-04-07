/*
 * Example program for SOS
 */

#include <SPI.h>
#include "sos.h"

#define	PROCESS_1_name	"light"
#define	PROCESS_1_title	"light"
#define	PROCESS_1_rt	"light"
#define	PROCESS_2_name	"temp"
#define	PROCESS_2_title	"temperature"
#define	PROCESS_2_rt	"celcius"
#define	PROCESS_3_name	"led"
#define	PROCESS_3_title	"led"
#define	PROCESS_3_rt	"light"

// MTU is less than 0.25 * (free memory in SRAM after initialization)
#define	MTU		200

int tmp_sensor = A0 ;
int light_sensor = A1 ;
int led = A2 ;

l2addr *myaddr = new l2addr_eth ("00:01:02:03:04:05") ;
l2net_eth e ;
Sos *sos ;

uint8_t process_light (Message &in, Message &out) 
{
    Serial.println (F ("process_light")) ;

    char message[10] ;
    for (int i (0) ; i < 10 ; i++)
	message[i] = '\0' ;

    int sensorValue = analogRead (light_sensor) ;
    snprintf (message, 10, "%d", sensorValue) ;

    out.set_payload ( strlen (message), (unsigned char *) message) ;

    out.set_code (COAP_RETURN_CODE (2,5)) ;

    return 0 ;
}

uint8_t process_temp (Message &in, Message &out) 
{
    Serial.println (F ("process_temp")) ;

    char message[10] ;
    for (int i (0) ; i < 10 ; i++)
	message[i] = '\0' ;

    int sensorValue = analogRead (tmp_sensor) ;
    snprintf (message, 10, "%d", sensorValue) ;

    out.set_payload ( strlen (message), (unsigned char *) message) ;

    out.set_code (COAP_RETURN_CODE (2,5)) ;

    return 0 ;
}

uint8_t process_led (Message &in, Message &out) 
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
    pinMode (light_sensor, INPUT) ;     
    pinMode (led, OUTPUT) ;     

    Serial.begin(38400) ;
    Serial.println (F ("start")) ;

    e.start (myaddr, false, MTU, SOS_ETH_TYPE) ;
    sos = new Sos (&e, 169) ;

    Resource *r1 = new Resource (PROCESS_1_name, PROCESS_1_title, PROCESS_1_rt) ;
    r1->add_handler (COAP_CODE_GET,process_light) ;
    sos->register_resource (r1) ;
    
    Resource *r2 = new Resource (PROCESS_2_name, PROCESS_2_title, PROCESS_2_rt) ;
    r2->add_handler (COAP_CODE_GET,process_temp) ;
    sos->register_resource (r2) ;
    
    Resource *r3 = new Resource (PROCESS_3_name, PROCESS_3_title, PROCESS_3_rt) ;
    r3->add_handler (COAP_CODE_GET,process_led) ;
    sos->register_resource (r3) ;
}

void test_values_temp (void)
{
    Message in, out ;
    process_temp (in, out) ;
    out.print () ;
}

void test_values_light (void)
{
    Message in, out ;
    process_light (in, out) ;
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

    Message in, out ;
    Message in2 ;

    in.push_option (opt_high) ;
    process_led (in, out) ;

    delay (500) ;
    in2.push_option (opt_low) ;
    process_led (in2, out) ;
}

void test_values (void)
{
    test_values_temp () ;
    test_values_light () ;
    test_values_led () ;
    delay (500) ;
}

void loop () 
{
    static int n = 0 ;

    if (n++ % 100000 == 0)
    {
	PRINT_DEBUG_STATIC ("\033[36m\tloop\033[00m") ;
	// check memory leak
	PRINT_FREE_MEM ;
	n = 1 ;
    }

    //test_values () ;
    sos->loop () ;
}
