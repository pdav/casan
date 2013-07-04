#NO_CORE = a
ARDUINO_DIR   = /usr/share/arduino
ARDMK_DIR     = /usr
AVR_TOOLS_DIR = /usr

TARGET			= blink
MCU				= atmega328p
F_CPU			= 16000000L
ARDUINO_PORT	= /dev/ttyACM0
UPLOAD_RATE		= 9600

ARDUINO_LIBS	= SPI mem

include /usr/share/arduino/Arduino.mk

test : all upload
	screen -s 9600 $(ARDUINO_PORT)
