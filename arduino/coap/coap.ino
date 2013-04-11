/*
	Don't forget the controler only have 2Kb of RAM.
*/
#include <SPI.h>
#include <EthernetRaw.h>
#include <Coap.h>
#include <utility/w5100.h>
#include <avr/wdt.h>

uint8_t eth_type[] = { 0x42, 0x42 };
uint8_t mac_addr[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };  
uint8_t mac_src[] = { 0x00, 0x22, 0x68, 0x32, 0x10, 0xf7 }; // the master
Coap *coap;

void setup()
{
	wdt_disable();
	Serial.begin (9600) ;
	Serial.println(F("start"));
	coap = new Coap(mac_addr, eth_type);
}

void loop()
{
	Serial.println(F("loop"));
	uint8_t rx_length = coap->coap_available();
	if(rx_length > 0)
	{
		uint8_t length;
		uint8_t token[10];
		uint8_t token_length;
		uint8_t id[2];
		uint8_t type;
		uint8_t code;
		uint8_t *payload;
		{
			uint8_t ret;
			ret = coap->decode_individual_coap(mac_src, &type, &code, 
					id, &token_length, token, &length, &payload);

			/*
			switch(ret)
			{
				case 1 : Serial.println(F("mac src")); break;
				case 2 : Serial.println(F("mac dest")); break;
				case 3 : Serial.println(F("eth type")); break;
				default : Serial.println(F("ok")); break;
			}
			*/
			if(ret != 0)
			{
				delay(1000);
				return;
			}
		}

		Serial.print(F("id "));
		Serial.print(id[0],HEX);
		Serial.print(':');
		Serial.println(id[1],HEX);
		Serial.print(F("type "));
		Serial.println(type, HEX);
		Serial.print(F("token_length "));
		Serial.println(token_length, HEX);
		Serial.print(F("token "));
		{
			int i ;
			for (i = 0 ; i < token_length ; i++)
			{
				Serial.print (token[i], HEX);
				Serial.print(':');
			}
			Serial.println();
		}
		Serial.print(F("code "));
		Serial.println(code, HEX);
		Serial.print(F("payloadlen "));
		Serial.println(length);		// TODO : FIXME the length isn't the right
		Serial.print(F("payload "));

		{	// We display the payload
			int i ;
			for (i = 0 ; i < length ; i++)
			{
				Serial.print (*(payload + i) , HEX);
				Serial.print(':');
			}
			Serial.println();
		}
		coap->emit_individual_coap(mac_src, type, code, id, token_length, token, length, payload );
	}
	
	delay(1000);
}
