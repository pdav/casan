ARDMK_DIR		= $(realpath ../../mk)
ARDUINO_DIR		= /usr/share/arduino
AVR_TOOLS_DIR		= /usr

TARGET			= .

#### For Arduino Uno platform
BOARD_TAG		= uno
# no need for ARDUINO_CORE_PATH	for uno
MONITOR_PORT		= /dev/ttyACM3
ARDUINO_LIBS		= Sos L2-eth SPI Ethernet
L2			= -DL2_ETH

#### For Zigduino r2 platform
BOARD_TAG		= zigduino_r2
ARDUINO_CORE_PATH	= $(ARDUINO_DIR)/hardware/arduino/cores/zigduino
MONITOR_PORT		= /dev/ttyUSB0
ARDUINO_LIBS		= Sos L2-154 ZigMsg
L2			= -DL2_154

USER_LIB_PATH		= $(realpath ../../libraries)

include $(ARDMK_DIR)/Arduino.mk

CPPFLAGS		+= -U__PROG_TYPES_COMPAT__ $(L2)

test : all upload monitor

retest: reset monitor
