/*
   Don't forget the controler only have 2Kb of RAM.
   */
#include <SPI.h>
#include "sos.h"
#include <avr/wdt.h>

#define PROCESS_1	"light"
#define PROCESS_2	"temp"

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
	sos->register_resource(PROCESS_1, sizeof PROCESS_1 -1, process_light);
	sos->register_resource(PROCESS_2, sizeof PROCESS_2 -1, process_temp);
}

// To do this test, you have to put "_rmanager" in public in sos.h
void test_get_resources(void)
{
	Message out; // we will get the resources list in the payload

	sos->_rmanager->get_all_resources(out);

	out.print();
}

void loop() {
	PRINT_DEBUG_STATIC("\033[36m\tloop \033[00m ");
	// to check if we have memory leak
	PRINT_FREE_MEM;
	test_get_resources();
	delay(1000);
}
