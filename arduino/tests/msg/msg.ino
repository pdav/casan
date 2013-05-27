/*
   Don't forget the controler only have 2Kb of RAM.
   */
#include <SPI.h>
#include "sos.h"
#include <avr/wdt.h>

#define PATH_1	"/chemin1"
#define PATH_2	"/chemin2"

#define URIQUERY1 "uriquery=1"
#define URIQUERY2 "uriquery=2"
#define URIQUERY3 "uriquery=3"

void setup() {
	/*
	   wdt_disable();
	   */
	Serial.begin (9600) ;
	Serial.println(F("start"));

}

void test_msg(void) {
	/* PRINT_DEBUG_STATIC("AVANT TOUT"); */
	Message m1;
	m1.print();
	Message m2;
	m2.print();
	PRINT_DEBUG_STATIC("ENSUITE");
	option uri_path1 (option::MO_Uri_Path, (void *) PATH_1, sizeof PATH_1 - 1);
	option uri_path2 (option::MO_Uri_Path, (void *) PATH_2, sizeof PATH_2 - 1);
	option os1 (option::MO_Uri_Query, (void *) URIQUERY1, sizeof URIQUERY1 - 1);
	option os2 (option::MO_Uri_Query, (void *) URIQUERY2, sizeof URIQUERY2 - 1);
	option os3 (option::MO_Uri_Query, (void *) URIQUERY3, sizeof URIQUERY3 - 1);

	/*
	PRINT_DEBUG_STATIC("AVANT");
	PRINT_FREE_MEM;
	PRINT_DEBUG_STATIC("OS3"); os3.print();
	os3 = os1;
	PRINT_DEBUG_STATIC("APRÈS os3 = os1;");
	PRINT_FREE_MEM;
	PRINT_DEBUG_STATIC("OS3"); os3.print();
	os3 = os2;
	PRINT_DEBUG_STATIC("APRÈS os3 = os2;");
	PRINT_FREE_MEM;
	PRINT_DEBUG_STATIC("OS3"); os3.print();
	os1 = uri_path1;
	PRINT_DEBUG_STATIC("APRÈS os1 = uri_path1;");
	PRINT_FREE_MEM;
	PRINT_DEBUG_STATIC("OS1"); os1.print();
	os2 = os1;
	PRINT_DEBUG_STATIC("APRÈS os2 = os1;");
	PRINT_FREE_MEM;
	PRINT_DEBUG_STATIC("OS2"); os2.print();
	os3 = os2;
	PRINT_DEBUG_STATIC("APRÈS os3 = os2;");
	PRINT_FREE_MEM;
	PRINT_DEBUG_STATIC("OS3"); os3.print();

	os1.print();
	os2.print();
	os3.print();
	uri_path1.print();
	uri_path2.print();
	*/

	// REGISTER message
	m1.set_type (COAP_TYPE_NON) ;
	m1.set_code (COAP_METHOD_POST) ;
	m1.set_id (10) ;

	PRINT_DEBUG_STATIC("M1 add os2");
	m1.push_option (os2) ;
	m1.print();	Serial.println();

	PRINT_DEBUG_STATIC("M1 add uri_path1");
	m1.push_option (uri_path1) ;
	m1.print();	Serial.println();

	PRINT_DEBUG_STATIC("M1 add uri_path2");
	m1.push_option (uri_path2) ;
	m1.print();	Serial.println();

	PRINT_DEBUG_STATIC("M1 add os1");
	m1.push_option (os1) ;
	m1.print();	Serial.println();

	PRINT_DEBUG_STATIC("M1 add os3");
	m1.push_option (os3) ;
	m1.print();	Serial.println();

	delay(1000);
	m2.set_type (COAP_TYPE_NON) ;
	m2.set_code (COAP_METHOD_POST) ;
	m2.set_id (11) ;

	PRINT_DEBUG_STATIC("M2 add os2");
	m2.push_option (os2) ;
	m2.print();	Serial.println();

	if( option::get_errno() != 0) {
		Serial.print(F("ERROR : ERRNO => "));
		Serial.println(option::get_errno());
		option::reset_errno();
	}


	return ;
}

void loop() {
	PRINT_DEBUG_STATIC("\033[36m\tloop \033[00m ");
	// to check if we have memory leak
	PRINT_FREE_MEM;
	test_msg();

	delay(1000);
}
