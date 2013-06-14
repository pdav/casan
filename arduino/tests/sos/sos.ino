/*
   Don't forget the controler only have 2Kb of RAM.
   */
#include <SPI.h>
#include "sos.h"
#include <avr/wdt.h>

#define PROCESS_1_name	"/light"
#define PROCESS_1_title	"light"
#define PROCESS_1_rt	"light"
#define PROCESS_2_name	"/temp"
#define PROCESS_2_title	"temperature"
#define PROCESS_2_rt	"°c"
#define PROCESS_3_name	"/led"
#define PROCESS_3_title	"led"
#define PROCESS_3_rt	"light"

int tmp_sensor = A0;
int light_sensor = A1;
int led = A2;

Sos *sos;
l2addr *mac_addr = new l2addr_eth("00:01:02:03:04:05");

uint8_t process_light(Message &in, Message &out) {
	Serial.println(F("process_light"));

	char message[10];
	for(int i(0) ; i < 10 ; i++)
		message[i] = '\0';

	int sensorValue = analogRead(light_sensor);
	snprintf(message, 10, "%d\0", sensorValue);

	option o (option::MO_Uri_Query,
		(unsigned char*) message,
		strlen(message)) ;

	out.set_code(COAP_RETURN_CODE(2,5));
	out.push_option(o);

	return 0;
}

uint8_t process_temp(Message &in, Message &out) {
	Serial.println(F("process_temp"));

	char message[10];
	for(int i(0) ; i < 10 ; i++)
		message[i] = '\0';

	int sensorValue = analogRead(tmp_sensor);
	snprintf(message, 10, "%d\0", sensorValue);

	option o (option::MO_Uri_Query,
		(unsigned char*) message,
		strlen(message)) ;

	out.set_code(COAP_RETURN_CODE(2,5));
	out.push_option(o);

	return 0;
}

uint8_t process_led(Message &in, Message &out) {
	Serial.println(F("process_led"));

	for(option * o = in.get_option() ; o != NULL ; o = in.get_option()) 
	{
		if (o->optcode() == option::MO_Uri_Query)
		{
			int n;
			if (sscanf ((const char *) o->val(), "%d", &n) == 1)
			{
				analogWrite(led, n);
			}
		}
	}

	out.set_code(COAP_RETURN_CODE(2,5));

	return 0;
}

void setup() {
	/*
	   wdt_disable();
	   */
	pinMode(tmp_sensor, INPUT);     
	pinMode(light_sensor, INPUT);     
	pinMode(led, OUTPUT);     

	Serial.begin (9600) ;
	Serial.println(F("start"));

	sos = new Sos(mac_addr, 169);
	sos->register_resource(PROCESS_1_name, sizeof PROCESS_1_name -1, 
			PROCESS_1_title, sizeof PROCESS_1_title -1, 
			PROCESS_1_rt, sizeof PROCESS_1_rt -1, 
			process_light);
	sos->register_resource(PROCESS_2_name, sizeof PROCESS_2_name -1, 
			PROCESS_2_title, sizeof PROCESS_2_title -1, 
			PROCESS_2_rt, sizeof PROCESS_2_rt -1, 
			process_temp);
	sos->register_resource(PROCESS_3_name, sizeof PROCESS_3_name -1, 
			PROCESS_3_title, sizeof PROCESS_3_title -1, 
			PROCESS_3_rt, sizeof PROCESS_3_rt -1, 
			process_led);
}


void test_values_temp(void)
{
	Message in, out;
	process_temp(in, out);
	out.print();
}

void test_values_light(void)
{
	Message in, out;
	process_light(in, out);
	out.print();
}

void test_values_led(void)
{
	char h[10];
	char l[10];

	snprintf(h, 10, "%d\0", 1024);
	snprintf(l, 10, "%d\0", 0);

	Serial.println(h);
	Serial.println(l);

	option opt_high (option::MO_Uri_Query, (unsigned char*) h, strlen(h)) ;
	option opt_low (option::MO_Uri_Query, (unsigned char*) l, strlen(l)) ;

	Message in, out;
	Message in2;

	in.push_option(opt_high);
	process_led(in, out);

	delay(500);
	in2.push_option(opt_low);
	process_led(in2, out);
}

void test_values(void)
{
	test_values_temp();
	test_values_light();
	test_values_led();
	delay(500);
}

void loop() 
{
	PRINT_DEBUG_STATIC("\033[36m\tloop\033[00m");
	// to check if we have memory leak
	PRINT_FREE_MEM;
	//test_values();
	sos->loop();
}
