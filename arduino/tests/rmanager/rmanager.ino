/*
   Don't forget the controler only have 2Kb of RAM.
   */
#include <SPI.h>
#include "sos.h"
#include <avr/wdt.h>

#define PROCESS_1_name	"/light"
#define PROCESS_1_title	"light"
#define PROCESS_1_rt	"light"
#define PROCESS_2_name	"/temp"
#define PROCESS_2_title	"temperature"
#define PROCESS_2_rt	"°c"

Sos *sos;
l2addr *mac_addr = new l2addr_eth("00:01:02:03:04:05");

uint8_t process_light(Message &in, Message &out) {
	Serial.println(F("process_light"));
	return 0;
}

uint8_t process_temp(Message &in, Message &out) {
	Serial.println(F("process_temp"));
	return 0;
}

void setup() {
	/*
	   wdt_disable();
	   */
	Serial.begin (9600) ;
	Serial.println(F("start"));

	sos = new Sos(mac_addr, 169);
	sos->register_resource(PROCESS_1_name, sizeof PROCESS_1_name -1, 
			PROCESS_1_title, sizeof PROCESS_1_title -1, 
			PROCESS_1_rt, sizeof PROCESS_1_rt -1, 
			process_light);
	sos->register_resource(PROCESS_2_name, sizeof PROCESS_2_name -1, 
			PROCESS_2_title, sizeof PROCESS_2_title -1, 
			PROCESS_2_rt, sizeof PROCESS_2_rt -1, 
			process_temp);
}

// To do this test, you have to put "rmanager_" in public in sos.h
void test_get_resources(void)
{
	Message out; // we will get the resources list in the payload

	sos->rmanager_->get_all_resources(out);

	out.print();
}

void test_request_resource(void)
{
	Message in; // we simule a received packet
	Message out; // we will get the resources list in the payload

	option o (option::MO_Uri_Query, 
			(void*)SOS_RESOURCES_ALL, 
			sizeof SOS_RESOURCES_ALL -1);

	in.set_type(COAP_TYPE_NON);
	in.set_code(COAP_CODE_GET);
	in.push_option(o);
	//in.set_payload(sizeof SOS_RESOURCES_ALL -1, (uint8_t*) SOS_RESOURCES_ALL);

	uint8_t ret = sos->rmanager_->request_resource(in, out);
	Serial.print(F("\033[36m request_resource returns : "));
	Serial.println(ret);

	out.print();
}

void loop() {
	PRINT_DEBUG_STATIC("\033[36m\tloop \033[00m ");
	// to check if we have memory leak
	PRINT_FREE_MEM;
	//test_get_resources();
	test_request_resource();
	delay(1000);
}
