#ifndef __SOS_H__
#define __SOS_H__

#include "Arduino.h"
#include "util.h"
#include "defs.h"
#include "enum.h"
#include "option.h"
#include "ethernetraw.h"
#include "coap.h"
#include "retransmit.h"
#include "rmanager.h"
#include "memory_free.h"
#include "time.h"


#define SOS_BUF_LEN			50
#define SOS_DELAY_INCR		50
#define SOS_DELAY_MAX		2000
#define SOS_DELAY			1500
#define SOS_DEFAULT_TTL		30000
#define SOS_RANDOM_UUID()	1337
#define	SOS_ETH_TYPE		0x88b5		// public use for prototype

class Sos {
	public:

		Sos(l2addr *mac_addr, long int uuid);
		~Sos();

		// reset address of the master (broadcast)
		// and set _hlid to 0
		void reset_master (void) ;	

		void register_resource(char *name, int namelen,
				char *title, int titlelen,
				char *rt, int rtlen,
				uint8_t (*handler)(Message &in, Message &out) );

		void reset (void);
		void loop();

		Rmanager * _rmanager = NULL;

	private:
		bool is_ctl_msg (Message &m);
		bool is_hello(Message &m);
		bool is_associate (Message &m);

		void mk_ctl_msg (Message &m);
		void mk_registration();
		void mk_ack_assoc(Message &in, Message &out);

		void print_coap_ret_type(eth_recv_t ret);

		Retransmit * _retransmition_handler = NULL;
		Coap *_coap = NULL;
		l2addr *_master_addr;
		EthernetRaw *_eth = NULL;
		uint8_t _status;
		long int _nttl;				// TTL get by assoc msg
		long int _hlid;				// hello ID
		int _current_message_id;
		long int _uuid;
		unsigned long _next_time_increment = SOS_DELAY;
		time _next_register;
		time _ttl_timeout;
		time _ttl_timeout_mid; // nttl /2
};

#endif
