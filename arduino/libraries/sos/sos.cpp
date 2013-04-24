#include "sos.h"

Sos::Sos() {
	set_status(SL_COLDSTART);
}

void Sos::set_l2(EthernetRaw *e) {
	_coap->set_l2(e);
}

void Sos::regiter_resource(char *name, uint8_t (*handler)(Message &in, Message &out) ) {
	_rmanager->add_resource(name, handler);
}

void Sos::set_status(enum status s) {
	_status = s;
}

void Sos::reset (void) {
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

void Sos::loop() {
	timeout_handler();
	if(_coap->coap_available()) {
		_coap->fetch();
		Serial.println(F("On vient de fetch"));
	}
	switch(_status) {
		case SL_COLDSTART :
			send_register();
			set_status(SL_WAITING_UNKNOWN);
			break;
		case SL_RUNNING :
			deduplicate();
			break;
		default :
	}
}

void Sos::deduplicate() {
}
