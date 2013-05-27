/*
   Don't forget the controler only have 2Kb of RAM.
   */
#include <SPI.h>
#include "sos.h"
#include <avr/wdt.h>

EthernetRaw * eth ;
l2addr *mac_addr = new l2addr_eth("00:01:02:03:04:05");
l2addr *mac_master = new l2addr_eth("00:22:68:32:10:f7");


void setup() {
	Serial.begin (9600) ;
	Serial.println(F("start"));

	eth = new EthernetRaw();
	eth->set_master_addr(NULL);
	eth->set_mac(mac_addr);
	eth->set_ethtype((int) SOS_ETH_TYPE);

}

void test_eth(void) {
	if( eth->recv() == ETH_RECV_RECV_OK ) {
		PRINT_DEBUG_STATIC("La taille du payload eth : ");
		PRINT_DEBUG_DYNAMIC(eth->get_payload_length());
	}
}

void loop() {
	PRINT_DEBUG_STATIC("\033[36m\tloop \033[00m ");
	// to check if we have memory leak
	PRINT_FREE_MEM;

	test_eth();

	delay(1000);
}
