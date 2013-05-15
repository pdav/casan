#ifndef __SOS_H__
#define __SOS_H__

#include "Arduino.h"
#include "util.h"
#include "defs.h"
#include "ethernetraw.h"
#include "coap.h"
#include "retransmit.h"
#include "rmanager.h"
#include "enum.h"
#include "memory_free.h"


#define SOS_BUF_LEN			50
#define SOS_DELAY_INCR		500
#define SOS_DELAY_MAX		10000
#define SOS_DELAY			1000
#define SOS_DEFAULT_TTL		30000
#define SOS_RANDOM_UUID()	1337
#define	SOS_ETH_TYPE		0x88b5		// public use for prototype

class Sos {
	public:

		Sos(l2addr *mac_addr, uint8_t uuid);
		~Sos();

		void set_master_addr (l2addr *a) ;	// set address
		void set_ttl (int ttl);	// set ttl
		void set_status(enum sos_slave_status);

		void registration();
		void register_resource(char *name, 
			uint8_t (*handler)(Message &in, Message &out) );

		void reset (void);
		void loop();

	private:

		Rmanager * _rmanager = NULL;
		Retransmit * _retransmition_handler = NULL;
		Coap *_coap = NULL;
		l2addr *_master_addr;
		EthernetRaw *_eth = NULL;
		uint8_t _status;
		uint8_t _ttl;
		int _current_message_id;
		uint8_t _uuid;
};

#endif
