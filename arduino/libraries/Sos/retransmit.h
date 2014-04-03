#ifndef __RETRANSMIT_H__
#define __RETRANSMIT_H__

/*
 * Retransmission handling
 *
 * This class provides support for a list of messages to retransmit
 * The loop_rentransmit function must be called periodically in order
 * to retransmit and/or expire messages.
 */

#include "defs.h"
#include "enum.h"
#include "coap.h"
#include "message.h"
#include "l2eth.h"
#include "memory_free.h"
#include "time.h"

#define DEFAULT_TIMER 4000

class Retransmit {
    public:
	Retransmit (Coap *c, l2addr **master) ;
	~Retransmit (void) ;

	void add (Message *m) ;		// add a message in retrans queue
	void del (Message *m) ;		// delete a message from retrans queue
	void loop_retransmit (void) ;	// to be called regularly
	void reset (void) ;
	void check_msg_received (Message &in) ;
	void check_msg_sent (Message &in) ;

    private:
	struct retransq
	{
	    Message *m ;
	    time timelast ;			// time of last transmission
	    time timenext ;			// time of next transmission
	    uint8_t ntrans ;			// # of retransmissions 
	    retransq *next ;			// next in queue
	} ;

	void del (retransq *r) ;
	void del (retransq *prev, retransq *cur) ;
	retransq *get_retransmit (Message *m) ;

	retransq *retransq_ ;
	Coap  *coap_ ;
	l2addr **master_addr_ ;
} ;

#endif
