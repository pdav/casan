/**
 * @file coap.h
 * @brief Some CoAP related definitions common to SOS.
 *	Some of these may not be used at all
 */

#ifndef	SOS_COAP_H
#define	SOS_COAP_H

namespace sos {

/*
 * SOS common CoAP definitions
 *
 */

/** SOS is based on this CoAP version */
#define	SOS_VERSION	1

#define	COAP_MKCODE(class,detail)	((((class)&0x7)<<5)|((detail)&0x1f))

/** maximum token length */
#define	COAP_MAX_TOKLEN	8

/*
 * CoAP constants
 */

/** ACK timeout (milliseconds) for CONfirmable messages */
#define	ACK_TIMEOUT	2000
/** ACK random factor to compute initial timeout for CON messages */
#define	ACK_RANDOM_FACTOR	1.5
/** maximum number of retransmissions */
#define	MAX_RETRANSMIT	4
/** maximum number of outstanding interactions (see coap draft 4.7) */
/** (outstanding interact. = CON without ACK or request without ACK or response) */
#define	NSTART		1
/** upper bound for random delay before responding to a multicast request */
#define	DEFAULT_LEISURE	5000
/** Maximum probing rate (in bytes/sec) */
#define	PROBING_RATE	1
/** max time from the first transmission of a CON message to the last */
#define	MAX_TRANSMIT_SPAN	(int (ACK_TIMEOUT*((1<<MAX_RETRANSMIT)-1)*ACK_RANDOM_FACTOR))
/** max time rom the first transmission of a CON msg to time when sender gives up */
#define	MAX_TRANSMIT_WAIT	(int (ACK_TIMEOUT*((1<<(MAX_RETRANSMIT+1))-1)*ACK_RANDOM_FACTOR))
/** default MAX_LATENCY is way too high for SOS */
#define	DEFAULT_MAX_LATENCY	1000
/** processing delay from CON reception to the ACK send */
#define	PROCESSING_DELAY	ACK_TIMEOUT
/** maximum round-trip time */
#define	MAX_RTT(maxlat)		(2*(maxlat)+PROCESSING_DELAY)
/** time from starting to send a CON to the time when an ACK is no longer expected */
#define	EXCHANGE_LIFETIME(maxlat)	(MAX_TRANSMIT_SPAN+(2*(maxlat))+PROCESSING_DELAY)
/** time from sending a Non-confirmable message to the time its Message ID can be safely reused */
#define	NON_LIFETIME(maxlat)	(MAX_TRANSMIT_SPAN+(maxlat))

}					// end of namespace sos
#endif
