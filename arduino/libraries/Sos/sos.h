#ifndef __SOS_H__
#define __SOS_H__

#include "Arduino.h"
#include "defs.h"
#include "enum.h"
#include "option.h"
#include "l2.h"
#include "l2eth.h"
#include "msg.h"
#include "retransmit.h"
#include "rmanager.h"
#include "memory_free.h"
#include "time.h"
#include "resource.h"

#define SOS_BUF_LEN		50
#define SOS_DELAY_INCR		50
#define SOS_DELAY_MAX		2000
#define SOS_DELAY		1500
#define SOS_DEFAULT_TTL		30000
#define SOS_RANDOM_UUID()	1337
#define	SOS_ETH_TYPE		0x88b5		// public use for prototype

class Sos {
    public:
	Sos (l2net *l2, long int uuid) ;
	~Sos () ;

	// reset address of the master (broadcast) and set hlid_ to 0
	void reset_master (void) ;	

	void register_resource (Resource *r) ;

	void reset (void) ;
	void loop () ;

	Rmanager *rmanager_ ;

    private:
	bool is_ctl_msg (Msg &m) ;
	bool is_hello (Msg &m) ;
	bool is_associate (Msg &m) ;

	void mk_ctl_msg (Msg &m) ;
	void mk_discover () ;
	void mk_ack_assoc (Msg &in, Msg &out) ;

	void print_coap_ret_type (l2_recv_t ret) ;
	void print_status (uint8_t status) ;

	Retransmit *retransmission_handler_ ;
	l2addr *master_ ;
	l2net *l2_ ;
	uint8_t status_ ;
	long int nttl_ ;			// TTL get by assoc msg
	long int hlid_ ;			// hello ID
	int current_message_id_ ;
	long int uuid_ ;
	unsigned long next_time_inc_ ;
	time next_register_ ;
	time ttl_timeout_ ;
	time ttl_timeout_mid_ ; 		// nttl /2
} ;

#endif
