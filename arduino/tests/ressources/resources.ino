/*
   Don't forget the controler only have 2Kb of RAM.
   */
#include <SPI.h>
#include "sos.h"
#include <avr/wdt.h>

#define PATH_WK ".well-known"
#define PATH_SOS "sos"


Rmanager * rmanager;
l2addr *mac_addr = new l2addr_eth("00:01:02:03:04:05");
l2addr *mac_master = new l2addr_eth("00:22:68:32:10:f7");

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

	rmanager = new Rmanager();
	rmanager->add_resource("light", 5 , process_light);
	rmanager->add_resource("temp", 4, process_temp);
}

// like we have message in & out
void test_resource_manager() {
	Message in;
	Message out;

	option uri_path1 (option::MO_Uri_Path, (void *) PATH_WK, sizeof PATH_WK - 1) ;
	option uri_path2 (option::MO_Uri_Path, (void *) PATH_SOS, sizeof PATH_SOS - 1) ;
    option ocf (option::MO_Content_Format, (void *) "abc", sizeof "abc" - 1) ;

	in.set_id(100);
	in.set_type(COAP_TYPE_ACK);
	in.push_option(uri_path1);
	in.push_option(uri_path2);
	in.push_option(ocf);

	in.print();
	rmanager->request_resource(in, out);
	out.print();
}

void loop() {
	PRINT_DEBUG_STATIC("\033[36m\tloop \033[00m ");
	// to check if we have memory leak
	PRINT_FREE_MEM;
	test_resource_manager();

	delay(1000);
}
