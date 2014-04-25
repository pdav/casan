/*
 * Test program for SOS resource management
 */

#include "sos.h"

#ifdef L2_ETH
    #include "l2-eth.h"

    l2addr *myaddr = new l2addr_eth ("00:01:02:03:04:05") ;
    l2net_eth l2 ;

    // MTU is less than 0.25 * (free memory in SRAM after initialization)
    #define	MTU	200
#endif

#ifdef L2_154
    #include "l2-154.h"

    l2addr *myaddr = new l2addr_154 ("be:ef") ;
    l2net_154 l2 ;

    #define	CHANNEL		25
    #define	PANID		CONST16 (0xca, 0xfe)
    #define	MTU		0
#endif

#define	DEBUGINTERVAL	5

#define R1_name		"light"
#define R1_title	"light"
#define R1_rt		"light"

#define R2_name		"temp"
#define R2_title	"temperature"
#define R2_rt		"°c"

#define PATH_WK		".well-known"
#define	PATH_SOS	"sos"

Sos *sos ;
Debug debug ;

int slaveid = 169 ;
bool promisc = false ;

const char *resname [] =
{
    "resources",
    "nonexistant",
    R1_name,
    R2_name,
} ;

uint8_t process_light (Msg &in, Msg &out)
{
    Serial.println (F ("process_light")) ;
    out.set_payload ((uint8_t *) "on", 2) ;		// light is "on"
    return COAP_RETURN_CODE (2, 5) ; ;
}

uint8_t process_temp (Msg &in, Msg &out)
{
    Serial.println (F ("process_temp")) ;
    out.set_payload ((uint8_t *) "23.5", 4) ;		// temp is hot ;-)
    return COAP_RETURN_CODE (2, 5) ;
}

void setup ()
{
    Serial.begin (38400) ;
    debug.start (DEBUGINTERVAL) ;

#ifdef L2_ETH
    l2.start (myaddr, promisc, MTU, ETHTYPE) ;
#endif
#ifdef L2_154
    l2.start (myaddr, promisc, MTU, CHANNEL, PANID) ;
#endif

    sos = new Sos (&l2, slaveid) ;

    Resource *r1 = new Resource (R1_name, R1_title, R1_rt) ;
    r1->add_handler (COAP_CODE_GET, process_light) ;
    sos->register_resource (r1) ;

    Resource *r2 = new Resource (R2_name, R2_title, R2_rt) ;
    r2->add_handler (COAP_CODE_GET, process_temp) ;
    sos->register_resource (r2) ;
}

void test_resource (const char *name)
{
    Msg in, out ;			// test with simulated messages

    Serial.print (F ("Resource: '")) ;
    Serial.print (name) ;
    Serial.print (F ("'")) ;
    Serial.println () ;

    option up (option::MO_Uri_Path, (void *) name, strlen (name)) ;
    option ocf (option::MO_Content_Format, (void *) "abc", sizeof "abc" - 1) ;

    in.set_id (100) ;
    in.set_type (COAP_TYPE_ACK) ;
    in.push_option (up) ;
    in.push_option (ocf) ;
    Serial.println (F ("Simulated message IN: ")) ;
    in.print () ;

    sos->request_resource (in, out) ;

    Serial.println (F ("Simulated message OUT: ")) ;
    out.print () ;
    Serial.println (F ("Done")) ;
}

void loop ()
{
    static int n = 0 ;

    if (debug.heartbeat ())
    {
	if (n % NTAB (resname) == 0)
	    sos->print_resources () ;
	test_resource (resname [n++ % NTAB (resname)]) ;
    }
}
