#include "sos.h"

extern l2addr_eth l2addr_eth_broadcast ;

// TODO : set all variables
Sos::Sos(l2addr *mac_addr, uint8_t uuid) 
: _ttl(SOS_DEFAULT_TTL), _status(SL_COLDSTART), _uuid(uuid) {
	_eth = new EthernetRaw();
	_master_addr = NULL;
	_eth->set_master_addr(_master_addr);
	_eth->set_mac(mac_addr);
	_eth->set_ethtype((int) SOS_ETH_TYPE);
	_current_message_id = 1;
	_coap = new Coap(_eth);
	_rmanager = new Rmanager();
	_retransmition_handler = new Retransmit(_coap, &_master_addr);
	set_status(SL_COLDSTART);
}

void Sos::set_master_addr(l2addr *a) {
	if(_master_addr != NULL)
		delete _master_addr;
	_master_addr = a;
	_eth->set_master_addr(_master_addr);
}

void Sos::register_resource(char *name, 
		uint8_t (*handler)(Message &in, Message &out) ) {
	_rmanager->add_resource(name, handler);
}

void Sos::set_status(enum sos_slave_status s) {
	_status = s;
}

// TODO : we need to restart all the application, 
// delete all the history of the exchanges
void Sos::reset (void) {
	set_status(SL_COLDSTART);
	_current_message_id = 0;
	_rmanager->reset();
	_retransmition_handler->reset();
	// This have to be the only time we delete the instance of master addr
	delete _master_addr;
	_master_addr = NULL;
}

void Sos::set_ttl (int ttl) {
	_ttl = ttl;
}

void Sos::registration() {
	Serial.println(F("registration"));
	PRINT_FREE_MEM
	l2addr * dest;
	if( _master_addr != NULL) {
		dest = _master_addr;
	} else {
		dest = &l2addr_eth_broadcast ;
		Serial.print(F("broadcast : "));
		l2addr_eth_broadcast.print();
	}
	Message m;
	{
		char message[SOS_BUF_LEN];
		snprintf(message, SOS_BUF_LEN, 
				"POST /.well-known/sos?register=%d", _uuid);
		m.set_payload(SOS_BUF_LEN, (unsigned char*) message);
	}
	m.set_type( COAP_TYPE_NON );
	m.set_id(_current_message_id);
	_coap->send(*dest, m);
}

// TODO : do the loop
void Sos::loop() {
	// to check all retrans. we have to do
	/* To check how many free memory we still have 
	   PRINT_FREE_MEM
	*/
	
	Serial.println(F("sos loop"));
	_retransmition_handler->loop_retransmit(); 
	Message in;
	Message out;
	int time_to_wait = SOS_DELAY;
	switch(_status) {

		case SL_COLDSTART :
			Serial.println(F("SL_COLDSTART"));
			delay(time_to_wait);
			registration();
			set_status(SL_WAITING_UNKNOWN);
			do {
				Serial.println(F("coldstart loop"));
				// TODO : we can check the coap recv return code
				// if we want to know how it fails
				while(_coap->coap_available()) {
					if (_coap->recv(in) == 0 ) {
						Serial.println(F("Recv message"));
						uint8_t * payload = in.get_payload();
						// FIXME : 
						// I don't really know how the hello mes. will be sent
						if(strncmp((const char*)payload, 
									"POST /.well-known/sos?hello=", 
									strlen("POST /.well-known/sos?hello=") 
								  ) == 0) {
							Serial.println(F("Recv HELLO"));
							_master_addr = new l2addr_eth();
							_coap->get_mac_src(_master_addr);
							_current_message_id = in.get_id() + 1;
							set_status(SL_WAITING_KNOWN);
						} else if (strncmp((const char*) payload, 
									"POST /.well-known/sos?associate=", 
									strlen("POST /.well-known/sos?associate=")
									) == 0) {
							Serial.println(F("Recv ASSOC"));
							// FIXME : maybe we should do some other tests
							if(in.get_id() == _current_message_id) {
								_master_addr = new l2addr_eth();
								_coap->get_mac_src(_master_addr);
								_current_message_id++;
								set_status(SL_RUNNING);
							}
						} else {
							Serial.println(F("Recv error"));
						}
					}
				}
				time_to_wait = 
					(time_to_wait + SOS_DELAY_INCR) < SOS_DELAY_MAX ? 
					time_to_wait + SOS_DELAY_INCR : time_to_wait ;
				delay(time_to_wait);
				registration();
			} while(_status == SL_WAITING_UNKNOWN);
			break;

		case SL_WAITING_KNOWN :
			// TODO
			Serial.println(F("SL_WAITING_KNOWN"));
			// like SL_WAITING_UNKNOWN we do not take into account
			// other messages than ALLOC & HELLO
			break;

		case SL_RENEW :
			// TODO
			Serial.println(F("SL_RENEW"));
			// TODO send message register & check the timer
			// at TTL = 0 : 
			// w. unknown & wait SOS_DELAY then broadcast a registration
			// if reception HELLO w. known then directed registration

		case SL_RUNNING :
			// TODO
			if(_coap->coap_available()) {
				_coap->recv(in);
				Serial.println(F("On vient de fetch"));
			}
			if(in.get_type() == COAP_TYPE_ACK) {
				_retransmition_handler->check_message(in); 
			}
			//	deduplicate();
			_rmanager->request_resource(in, out);
			break;

		default :
			Serial.println(F("Error : sos status not known"));
			break;
	}
}
