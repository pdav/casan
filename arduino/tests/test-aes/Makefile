ARDMK_DIR		= $(realpath ../../mk)
ARDUINO_DIR		= /usr/share/arduino
AVR_TOOLS_DIR		= /usr

TARGET			= .

#DBLEVEL		= -DDBLEVEL=0
DBLEVEL			= -DDBLEVEL=1

#### For Arduino Uno platform and Ethernet
# BOARD_TAG		= uno
# # no need for ARDUINO_CORE_PATH for uno
# MONITOR_PORT		= /dev/ttyACM3
# ARDUINO_LIBS		= not compatible with Uno

#### For Zigduino r2 platform and 802.15.4
BOARD_TAG		= zigduino_r2
ARDUINO_CORE_PATH	= $(ARDUINO_DIR)/hardware/arduino/cores/zigduino
MONITOR_PORT		?= /dev/ttyUSB0
ARDUINO_LIBS		= ZigMsg Dtls/aes Dtls/gf256mul

USER_LIB_PATH		= $(realpath ../../libraries)

# SCREEN_OPT

include $(ARDMK_DIR)/Arduino.mk

CPPFLAGS		+= -U__PROG_TYPES_COMPAT__ $(DBLEVEL)

test : all upload monitor

retest: reset monitor

redo: clean all
