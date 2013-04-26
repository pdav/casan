/*
	Don't forget the controler only have 2Kb of RAM.
*/
#include <SPI.h>
#include <sos.h>
#include <avr/wdt.h>

uint8_t eth_type[] = { 0x42, 0x42 };
//uint8_t mac_addr[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };  
//uint8_t mac_src[] = { 0x00, 0x22, 0x68, 0x32, 0x10, 0xf7 }; // the master
Sos *sos;
l2addr_eth mac_addr("00:01:02:03:04:05");
l2addr_eth mac_master("00:22:68:32:10:f7");

void setup() {
	/*
	wdt_disable();
	*/
	Serial.begin (9600) ;
	Serial.println(F("start"));

	EthernetRaw eth = new EthernetRaw();
	eth->set_mac(mac_addr);
	eth->set_ethtype(eth_type);

	sos = new sos();
	sos->set_l2(eth);
	sos->register_resource("/light", process_light);
	sos->register_resource("/temp", process_temp);
}

uint8_t process_light(Message &in, Message &out) {
	Serial.println(F("process_light"));
}

uint8_t process_temp(Message &in, Message &out) {
	Serial.println(F("process_temp"));
}


void loop() {
	Serial.println(F("loop"));

	sos->loop();

	delay(1000);
}
