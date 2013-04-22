#ifndef sos_h
#define sos_h

#include "Arduino.h"
#include "Coap.h"

#define SOS_BUF_LEN 50

class Sos {
	public:
		enum status {
			SL_COLDSTART,
			SL_WAITING_UNKNOWN,
			SL_RUNNING,
			SL_RENEW,
			SL_WAITING_KNOWN,
		} ;

		Sos();
		~Sos();
		/*
		void set_l2net (l2net *l2) ;	// set l2 network
		void set_l2addr (l2addr *a) ;	// set address
		*/

		void set_l2(EthernetRaw *e);
		void regiter_resource(char *name, 
			uint8_t (*handler)(enum request_method, uint8_t*) );
		void set_status(enum status);
		void reset (void);
		void set_ttl (int ttl);	// set ttl
		void loop();

	private:
		void timeout_handler();
		void deduplicate();

		/*
		l2net *_l2 ;			// l2 network this slave is on
		l2addr *_addr ;			// master address
		*/

		uint8_t *_master_addr;
		uint8_t _broadcast[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
		Rmanager * _rmanager;
		Coap *_coap;
		uint8_t _status;
		uint8_t _ttl;
		uint8_t _current_message_id;
		uint8_t uuid;
};

#endif
