#ifndef __RETRANS_H__
#define __RETRANS_H__

/*
 * Retransmission handling
 *
 * This class provides support for a list of messages to retransmit
 * The loop function must be called periodically in order to
 * retransmit and/or expire messages.
 */

#include "defs.h"
#include "enum.h"
#include "msg.h"
#include "l2.h"
#include "memory_free.h"
#include "time.h"

#define DEFAULT_TIMER 4000

class Retrans {
    public:
	Retrans (l2addr **master) ;
	~Retrans (void) ;

	void add (Msg *m) ;		// add a message in retrans queue
	void del (Msg *m) ;		// delete a message from retrans queue
	void loop (l2net &l2, time_t &curtime) ; // to be called regularly
	void reset (void) ;
	void check_msg_received (Msg &in) ;
	void check_msg_sent (Msg &in) ;

    private:
	struct retransq
	{
	    Msg *msg ;
	    time_t timelast ;		// time of last transmission
	    time_t timenext ;		// time of next transmission
	    uint8_t ntrans ;		// # of retransmissions 
	    retransq *next ;		// next in queue
	} ;

	void del (retransq *r) ;
	void del (retransq *prev, retransq *cur) ;
	retransq *get (Msg *m) ;

	retransq *retransq_ ;
	l2addr **master_addr_ ;
} ;

#endif
