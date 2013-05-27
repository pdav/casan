#ifndef __DEFS_H__
#define __DEFS_H__
/*
 * common definitions
 *
 */

// number of elements in an array
#define NTAB(t)		(sizeof (t)/sizeof (t)[0])

// http://stackoverflow.com/questions/3366812/linux-raw-ethernet-socket-bind-to-specific-protocol

// Ethernet address length
#define	ETHADDRLEN	6

// Ethernet default MTU
#define	ETHMTU		1536

// SOS is based on this CoAP version
#define	SOS_VERSION	1

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

#define ALEA(x) x

#define PRINT_FREE_MEM \
	Serial.print(F("\tfreeMemory() = \033[36m")); \
	Serial.print(freeMemory()); \
	Serial.println("\033[00m");

// to print a dynamic value (everything but strings)
#define PRINT_DEBUG_DYNAMIC(x) \
	Serial.println(x);

// to print a static value (string)
#define PRINT_DEBUG_STATIC(x) \
	Serial.println(F(x));

#define		BYTE_LOW(n)		((n) & 0xff)
#define		BYTE_HIGH(n)	(((n) & 0xff00) >> 8)
#define FORMAT_BYTE0(ver,type,toklen)               \                                                                                                                                                                                      
	(												\
		 (((unsigned int) (ver) & 0x3) << 6) |		\                                   
		 (((unsigned int) (type) & 0x3) << 4) |		\                                   
		 (((unsigned int) (toklen) & 0x7))			\                                       
	)                                                                           

#define	OPTVAL(o)	((o)->optval_ ? (o)->optval_ : (o)->staticval_)

#endif
