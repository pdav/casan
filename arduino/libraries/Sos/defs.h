#ifndef __DEFS_H__
#define __DEFS_H__
/*
 * common definitions
 *
 */

#include <Arduino.h>		// Really useful

// number of elements in an array
#define NTAB(t)		((int) (sizeof (t)/sizeof (t)[0]))

/*
 * Debug macros, which can be disabled to count program size
 */

#if DBLEVEL > 0
#  define DBG1(a)	Serial.print (a)
#  define DBG2(a,b)	Serial.print (a, b)
#  define DBGLN0()	Serial.println ()
#  define DBGLN1(a)	Serial.println (a)
#else
#  define DBG1(a)	/* nothing */
#  define DBG2(a,b)	/* nothing */
#  define DBGLN0()	/* nothing */
#  define DBGLN1(a)	/* nothing */
#endif

/*
 * ANSI escape sequences
 * See http://en.wikipedia.org/wiki/ANSI_escape_code
 */

#define C_CSI		"\033["
#define	C_RESET		C_CSI "m"

// normal colors
#define	C_BLACK		C_CSI "30m"
#define	C_RED		C_CSI "31m"
#define	C_GREEN		C_CSI "32m"
#define	C_YELLOW	C_CSI "33m"
#define	C_BLUE		C_CSI "34m"
#define	C_MAGENTA	C_CSI "35m"
#define	C_CYAN		C_CSI "36m"
#define	C_WHITE		C_CSI "37m"

// bright colors
#define	B_BLACK		C_CSI "30;1m"
#define	B_RED		C_CSI "31;1m"
#define	B_GREEN		C_CSI "32;1m"
#define	B_YELLOW	C_CSI "33;1m"
#define	B_BLUE		C_CSI "34;1m"
#define	B_MAGENTA	C_CSI "35;1m"
#define	B_CYAN		C_CSI "36;1m"
#define	B_WHITE		C_CSI "37;1m"

#define	RED(m)			B_RED m C_RESET
#define	BLUE(m)			C_BLUE m C_RESET
#define	YELLOW(m)		C_YELLOW m C_RESET

// CoAP ACK timeout (milliseconds) for CONfirmable messages
#define	ACK_TIMEOUT	2000
// CoAP ACK random factor to compute initial timeout for CON messages
#define	ACK_RANDOM_FACTOR	1.5
// CoAP maximum number of retransmissions
#define MAX_RETRANSMIT	4
// CoAP maximum number of outstanding interactions (see coap draft 4.7)
// (outstanding interact. = CON without ACK or request without ACK or response)
#define	NSTART		1
// CoAP upper bound for random delay before responding to a multicast request
#define	DEFAULT_LEISURE	5000
// Maximum probing rate (in bytes/sec)
#define	PROBING_RATE	1
// To sleep between 2 occurs of the loop, to prevent bugs from arduino (ms)
#define DELAY_TO_SLEEP	50

#define MAX_TRANSMIT_SPAN	 45		// seconds
#define MAX_TRANSMIT_WAIT	 93		// seconds
#define MAX_LATENCY      	100		// seconds
#define PROCESSING_DELAY 	  2		// seconds
#define MAX_RTT          	202		// seconds
#define EXCHANGE_LIFETIME	247		// seconds
#define NON_LIFETIME     	145		// seconds

#define ALEA(x) x

#define	BYTE_HIGH(n)		(((n) & 0xff00) >> 8)
#define	BYTE_LOW(n)		((n) & 0xff)
#define	INT16(h,l)		(((uint16_t) (h)) << 8 | (uint16_t) (l))
#endif
