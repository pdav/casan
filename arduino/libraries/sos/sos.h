#ifndef __SOS_H__
#define __SOS_H__

#include "Arduino.h"
#include "coap.h"
#include "rmanager.h"
#include "ethernetraw.h"
#include "l2eth.h"
#include "l2.h"
#include "message.h"
#include "retransmit.h"
#include "util.h"

#define SOS_BUF_LEN 50

class Sos {
	public:
		enum sos_slave_status {
			SL_COLDSTART,
			SL_WAITING_UNKNOWN,
			SL_RUNNING,
			SL_RENEW,
			SL_WAITING_KNOWN,
		} ;

		Sos();
		~Sos();

		void set_master_addr (l2addr *a) ;	// set address

		void set_l2(EthernetRaw *e);
		void regiter_resource(char *name, 
			uint8_t (*handler)(Message &in, Message &out) );
		void set_status(enum sos_slave_status);
		void reset (void);
		void set_ttl (int ttl);	// set ttl
		void loop();

	private:

		Rmanager * _rmanager = NULL;
		Retransmit * _retransmition_handler = NULL;
		Coap *_coap = NULL;
		uint8_t _status;
		uint8_t _ttl;
		uint8_t _current_message_id;
		uint8_t uuid;
};

#endif
