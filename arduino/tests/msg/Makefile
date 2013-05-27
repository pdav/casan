ARDUINO_DIR = /usr/share/arduino/
#NO_CORE = a

TARGET			= msg
#MCU			= atmega168
MCU				= atmega328p
#F_CPU			= 16000000
F_CPU			= 16000000L
#ARDUINO_PORT	= /dev/ttyUSB*
ARDUINO_PORT	= /dev/ttyACM0
UPLOAD_RATE		= 9600

ARDUINO_LIBS	= sos sos/utility SPI

include /usr/share/arduino/Arduino.mk

test : all upload
	screen -s 9600 /dev/ttyACM0
