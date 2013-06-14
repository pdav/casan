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


#define SOS_BUF_LEN			50
#define SOS_DELAY_INCR		50
#define SOS_DELAY_MAX		2000
#define SOS_DELAY			500
#define SOS_DEFAULT_TTL		30000
#define SOS_RANDOM_UUID()	1337
#define	SOS_ETH_TYPE		0x88b5		// public use for prototype

class Sos {
	public:

		Sos(l2addr *mac_addr, long int uuid);
		~Sos();

		void set_master_addr (l2addr *a) ;	// set address

		void register_resource(char *name, int namelen,
			uint8_t (*handler)(Message &in, Message &out) );

		void reset (void);
		void loop();

		Rmanager * _rmanager = NULL;
	private:
		bool is_ctl_msg (Message &m);
		void mk_ctl_msg (Message &m);
		void mk_assoc(Message &out);
		void mk_registration();
		void mk_ack_assoc(Message &in, Message &out);
		bool is_associate (Message &m);
		bool is_hello(Message &m);
		void mk_ttl_compute(void);

		Retransmit * _retransmition_handler = NULL;
		Coap *_coap = NULL;
		l2addr *_master_addr;
		EthernetRaw *_eth = NULL;
		uint8_t _status;
		long int _nttl;				// TTL get by assoc msg
		long int _ttl;				// TTL often recomputed
		long int _old_cur_time;		// last time ttl calcul
		long int _hlid;		// hello ID
		int _current_message_id;
		long int _uuid;
		int _time_to_wait = SOS_DELAY;
};

#endif
