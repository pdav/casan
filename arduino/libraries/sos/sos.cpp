#include "sos.h"

Sos::Sos() {
	set_status(SL_COLDSTART);
}

void Sos::set_l2(EthernetRaw *e) {
	_coap->set_l2(e);
}
void Sos::regiter_resource(char *name, uint8_t (*handler)(uint8_t, uint8_t*) ) {
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

// TODO
void Sos::send_register() {
	uint8_t * dest;
	if( _status == SL_WAITING_KNOWN) {
		dest = _master_addr;
	}
	else {
		dest = _broadcast;
	}
	char message[SOS_BUF_LEN];
	sprintf(message, "/.well-known/sos?register=%d", _uuid);
	_coap->emit_individual_coap( dest,
	COAP_MES_TYPE_NON, 
	COAP_CODE_POST, 
	_current_message_id++,
	0,			// token_length
	NULL,		// *token
	strlen(message),	// payload_len
	message);	// *payload
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
