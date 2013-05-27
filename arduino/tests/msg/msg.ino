/*
   Don't forget the controler only have 2Kb of RAM.
   */
#include <SPI.h>
#include "sos.h"
#include <avr/wdt.h>

#define PATH_1	"/chemin1"
#define PATH_2	"/chemin2"

#define URIQUERY "uriquery=123"
#define URIQUERY2 "1234567"

void setup() {
	/*
	   wdt_disable();
	   */
	Serial.begin (9600) ;
	Serial.println(F("start"));

}

void test_msg(void) {
	Message m;
	option uri_path1 (option::MO_Uri_Path, (void *) PATH_1, sizeof PATH_1 - 1);
	option uri_path2 (option::MO_Uri_Path, (void *) PATH_2, sizeof PATH_2 - 1);
	option uri_path3 (option::MO_Uri_Path, (void *) PATH_2, sizeof PATH_2 - 1);
	option os1 (option::MO_Uri_Query, (void *) URIQUERY, sizeof URIQUERY - 1);
	option os2 (option::MO_Uri_Query, (void *) URIQUERY2, sizeof URIQUERY2 - 1);
	option os3 (option::MO_Uri_Query, (void *) URIQUERY2, sizeof URIQUERY2 - 1);

	// REGISTER message
	m.set_type (COAP_TYPE_NON) ;
	m.set_code (COAP_METHOD_POST) ;
	m.set_id (10) ;
	m.push_option (uri_path1) ;
	m.push_option (uri_path2) ;
	m.push_option (uri_path3) ;
	m.push_option (os1) ;
	m.push_option (os2) ;
	m.push_option (os3) ;

	if( option::get_errno() != 0)
		PRINT_DEBUG_STATIC("ERROR : ERRNO");

	//m.print();
	return ;
}

void loop() {
	PRINT_DEBUG_STATIC("\033[36m\tloop \033[00m ");
	// to check if we have memory leak
	PRINT_FREE_MEM;
	test_msg();

	delay(1000);
}
