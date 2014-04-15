/*
 * Test program for SOS resource management
 */

#include <SPI.h>
#include "sos.h"

#define	DEBUGINTERVAL	10

#define R1_name		"/light"
#define R1_title	"light"
#define R1_rt		"light"

#define R2_name		"/temp"
#define R2_title	"temperature"
#define R2_rt		"°c"

l2addr *myaddr = new l2addr_eth ("00:01:02:03:04:05") ;
l2net_eth e ;
Sos *sos ;

bool promisc = false ;
int slaveid = 169 ;
int mtu = 200 ;

uint8_t process_light (Msg &in, Msg &out)
{
    Serial.println (F ("process_light")) ;
    return 0 ;
}

uint8_t process_temp (Msg &in, Msg &out)
{
    Serial.println (F ("process_temp")) ;
    return 0 ;
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


// To do this test, you have to put "rmanager_" in public in sos.h
void test_get_resources (void)
{
    Msg out ;		// resource list will be stored in the payload

    sos->rmanager_->get_resource_list (out) ;
    out.print () ;
}

void test_request_resource (void)
{
    Msg in ;		// simulation of a received packet
    Msg out ;		// resource list will be stored in the payload

    option o (option::MO_Uri_Query, 
		    (void*) SOS_RESOURCES_ALL, 
		    sizeof SOS_RESOURCES_ALL -1) ;

    in.set_type (COAP_TYPE_NON) ;
    in.set_code (COAP_CODE_GET) ;
    in.push_option (o) ;

    //in.set_payload (sizeof SOS_RESOURCES_ALL -1, (uint8_t*)SOS_RESOURCES_ALL) ;

    uint8_t ret = sos->rmanager_->request_resource (in, out) ;
    Serial.print (F ("\033[36m request_resource returns : ")) ;
    Serial.println (ret) ;

    out.print () ;
}

void loop ()
{
    if (debug.heartbeat ())
    {
	PRINT_FREE_MEM ;
	//test_get_resources () ;
	test_request_resource () ;
    }
}
