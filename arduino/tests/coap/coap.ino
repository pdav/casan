/*
   Don't forget the controler only have 2Kb of RAM.
   */
#include <SPI.h>
#include "sos.h"
#include <avr/wdt.h>

Coap *coap;
EthernetRaw * eth ;

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

	EthernetRaw * eth = new EthernetRaw();
	eth->set_master_addr(NULL); // mac_master
	eth->set_mac(mac_addr);
	eth->set_ethtype((int) SOS_ETH_TYPE);
	coap = new Coap(eth);

}

void test_coap(void) {
	Message in;
	Message out;
	enum eth_recv r ;
	int n ;

	n = 0 ;
	while((r = coap->recv(in)) != ETH_RECV_EMPTY) {
		n++ ;
		if (r == ETH_RECV_RECV_OK)
			in.print();
	}
	if (n > 0)
	{
		Serial.print (n) ;
		Serial.println (" msg absorbed") ;
	}
}

void loop() {
	// Serial.print(F("\033[36m\tloop \033[00m "));
	// to check if we have memory leak
	PRINT_FREE_MEM;

	test_coap();

	// delay(1000);
	delay(500);
}
