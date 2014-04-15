ARDMK_DIR		= $(realpath ../../mk)
ARDUINO_DIR		= /usr/share/arduino
AVR_TOOLS_DIR		= /usr

TARGET			= .
BOARD_TAG		= zigduino_r2

ARDUINO_CORE_PATH	= $(ARDUINO_DIR)/hardware/arduino/cores/zigduino

MONITOR_PORT		= /dev/ttyUSB1

USER_LIB_PATH		= $(realpath ../../libraries)
# ARDUINO_LIBS		= ZigMsg # sos sos/utility SPI
ARDUINO_LIBS		= ZigMsg

include $(ARDMK_DIR)/Arduino.mk

CPPFLAGS		+= -U__PROG_TYPES_COMPAT__ 

test : all upload monitor

retest: reset monitor
