/*
	Don't forget the controler only have 2Kb of RAM.
*/
#include <SPI.h>
#include "sos.h"
#include <avr/wdt.h>

//uint8_t mac_addr[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };  
//uint8_t mac_src[] = { 0x00, 0x22, 0x68, 0x32, 0x10, 0xf7 }; // the master
Sos *sos;
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

	sos = new Sos(mac_addr, SOS_ETH_TYPE, SOS_RANDOM_UUID());
	sos->register_resource("/light", process_light);
	sos->register_resource("/temp", process_temp);
}

void loop() {
	Serial.println(F("loop"));

	sos->loop();

	delay(1000);
}
