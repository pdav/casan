#include "sos.h"

// TODO : set all variables
Sos::Sos() {
	set_status(SL_COLDSTART);
}

void Sos::set_l2(EthernetRaw *e) {
	_coap->set_l2(e);
}

void Sos::set_master_addr(l2addr *a) {
	if(_master_addr != NULL)
		delete _master_addr;
	_master_addr = a;
}

void Sos::regiter_resource(char *name, uint8_t (*handler)(Message &in, Message &out) ) {
	_rmanager->add_resource(name, handler);
}

void Sos::set_status(enum sos_slave_status s) {
	_status = s;
}

// TODO : reset all the application
void Sos::reset (void) {
	_rmanager->reset();
	_retransmition_handler->reset();
}

void Sos::set_ttl (int ttl) {
	_ttl = ttl;
}

void Sos::reset (void) {
	set_status(SL_COLDSTART);
	// TODO : we need to restart all the application, delete all the history of the exchanges
}

void Sos::send_register() {
	uint8_t * dest;
	if( _status == SL_WAITING_KNOWN) {
		dest = _master_addr;
	}
	else {
		dest = _broadcast;
	}
	Message m;
	{
		char message[SOS_BUF_LEN];
		sprintf(message, "/.well-known/sos?register=%d", _uuid);
		m.set_payload(SOS_BUF_LEN, message);
	}
	m.set_type( COAP_MES_TYPE_NON );
	m.set_id(_current_message_id++);
	_coap->send(dest, m);
}

// TODO : do the loop
void Sos::loop() {
	_retransmition_handler->loop_retransmit(); // to check all retrans. we have to do
	Message in();
	Message out();
	if(_coap->coap_available()) {
		_coap->recv(&in);
		Serial.println(F("On vient de fetch"));
	}
	switch(_status) {
		case SL_COLDSTART :
			send_register();
			set_status(SL_WAITING_UNKNOWN);
			break;
		case SL_RUNNING :
		//	deduplicate();
			_rmanager->request_resource(in, out);
			break;
		default :
	}
	delete m;
}
