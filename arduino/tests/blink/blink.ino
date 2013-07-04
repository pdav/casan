/*
   Don't forget the controler only have 2Kb of RAM.
   */
#include <SPI.h>
#include <avr/wdt.h>
#include "memory_free.h"

int led = 13;

void setup() 
{
	/*
	   wdt_disable();
	   */

	Serial.begin (9600) ;
	Serial.println(F("start"));
	pinMode(led, OUTPUT);     

}

void loop() 
{
	Serial.println();
	// to check if we have memory leak
	PRINT_FREE_MEM;

	digitalWrite(led, HIGH);
	delay(1000);
	digitalWrite(led, LOW);
	delay(1000);

}
