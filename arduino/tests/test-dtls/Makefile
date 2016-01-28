ARDMK_DIR		= $(realpath ../../mk)
ARDUINO_DIR		= /usr/share/arduino
AVR_TOOLS_DIR		= /usr

TARGET			= .

#DBLEVEL		= -DDBLEVEL=0
DBLEVEL			= -DDBLEVEL=1

#### For Arduino Uno platform and Ethernet
#BOARD_TAG		= uno
## no need for ARDUINO_CORE_PATH for uno
#MONITOR_PORT		= /dev/ttyACM3
#ARDUINO_LIBS		= not compatible with Uno

#### For Zigduino r2 platform and 802.15.4
BOARD_TAG		= zigduino_r2
ARDUINO_CORE_PATH	= $(ARDUINO_DIR)/hardware/arduino/cores/zigduino
MONITOR_PORT		?= /dev/ttyUSB0
#ARDUINO_LIBS		= ZigMsg Dtls/aes Dtls/sha2 Dtls
ARDUINO_LIBS		?= ZigMsg Dtls-s/aes Dtls-s/sha2 Dtls-s

# changes the program behavior
CPPFLAGS  += '-DMSG_DELAY=50' # delay between two loops
CPPFLAGS  += '-DMSG_RECEIVED' # prints the received msg
CPPFLAGS  += '-DSIZE_TO_SEND=87' # msg size
#CPPFLAGS  += '-DSIZE_TO_SEND=1' # msg size
CPPFLAGS  += '-DHW_ENCRYPTION_ATMEGA128RFA1' # hardware encryption

# prints things
#CPPFLAGS += '-DMSG_DEBUG' 		# debug msg
#CPPFLAGS  += '-DMSG_FREEMEM' 		# free memory left
CPPFLAGS  += '-DMSG_DURATION' 		# msg exchange duration
CPPFLAGS  += '-DMSG_ENCRYPT_TIME' 	# encryption and decryption time
#CPPFLAGS  += '-DMESURE_TIME_START_PUSHING_MSG' # time spent before pushing data

CPPFLAGS  += -U__PROG_TYPES_COMPAT__ $(DBLEVEL)
CPPFLAGS  += -Wno-unused-variable -Wno-unused-function

USER_LIB_PATH		= $(realpath ../../libraries)

# SCREEN_OPT=-L => log everything
# MONITOR_PORT=/dev/ttyUSB1

include $(ARDMK_DIR)/Arduino.mk

test : all upload monitor

retest: reset monitor

redo: clean all
