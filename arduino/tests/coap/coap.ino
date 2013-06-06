/*
   Don't forget the controler only have 2Kb of RAM.
   */
#include <SPI.h>
#include "sos.h"
#include <avr/wdt.h>

#define	PATH_1		".well-known"
#define	PATH_2		"sos"
#define	PATH_3		"path3"

Coap *coap;
EthernetRaw * eth ;

l2addr *mac_addr = new l2addr_eth("00:01:02:03:04:05");
l2addr *mac_master = new l2addr_eth("00:22:68:32:10:f7");

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

void test_reception(void) {
	Message in;
	enum eth_recv r ;

	while((r = coap->recv(in)) != ETH_RECV_EMPTY) {
		if (r == ETH_RECV_RECV_OK)
			in.print();
	}
}

void test_emission(void) {
	Message m1;
	Message m2;
	option uri_path1 (option::MO_Uri_Path, (void *) PATH_1, sizeof PATH_1 - 1) ;
	option uri_path2 (option::MO_Uri_Path, (void *) PATH_2, sizeof PATH_2 - 1) ;
	option uri_path3 (option::MO_Uri_Path, (void *) PATH_3, sizeof PATH_3 - 1) ;
    option ocf (option::MO_Content_Format, (void *) "abc", sizeof "abc" - 1) ;

	m1.set_id(258);
	m1.set_type(COAP_TYPE_NON);
	m1.push_option(ocf);
	m1.push_option(uri_path1);
	m1.push_option(uri_path2);
	m1.push_option(uri_path3);
	coap->send(l2addr_eth_broadcast, m1);

	m2.set_id(33);
	m2.set_type(COAP_TYPE_CON);
	m2.push_option(ocf);
	coap->send(l2addr_eth_broadcast, m2);

}

void loop() {
	// Serial.print(F("\033[36m\tloop \033[00m "));
	// to check if we have memory leak
	PRINT_FREE_MEM;

	//test_reception();
	test_emission();

	delay(1000);
	//delay(500);
}
