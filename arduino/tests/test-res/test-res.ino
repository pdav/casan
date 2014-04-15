/*
 * Test program for SOS resource management
 */

#include "sos.h"

#include "l2-eth.h"

#define	DEBUGINTERVAL	5

#define R1_name		"light"
#define R1_title	"light"
#define R1_rt		"light"

#define R2_name		"temp"
#define R2_title	"temperature"
#define R2_rt		"°c"

#define PATH_WK		".well-known"
#define	PATH_SOS	"sos"

l2addr *myaddr = new l2addr_eth ("00:01:02:03:04:05") ;
l2net_eth e ;
Sos *sos ;

bool promisc = false ;
int slaveid = 169 ;
int mtu = 200 ;


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
    e.start (myaddr, promisc, mtu, SOS_ETH_TYPE) ;
    sos = new Sos (&e, slaveid) ;
    debug.start (DEBUGINTERVAL) ;

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

    sos->rmanager_->request_resource (in, out) ;

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
