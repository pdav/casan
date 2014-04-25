#ifndef __DEFS_H__
#define __DEFS_H__
/*
 * common definitions
 *
 */

// number of elements in an array
#define NTAB(t)		((int) (sizeof (t)/sizeof (t)[0]))

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
