#include "rmanager.h"

Rmanager::Rmanager() { 
}

Rmanager::~Rmanager() { 
	reset();
}

void Rmanager::add_resource(char *name, 
		uint8_t (*handler)(Message &in, Message &out)) {
	rmanager_s * newresource = (rmanager_s *) malloc(sizeof(rmanager_s));
	newresource->name = (char *) malloc(strlen(name) + 1);
	memcpy(newresource->name, name, strlen(name) + 1);
	newresource->h = handler;
	newresource->s = _resources;
	_resources = newresource;
}

void Rmanager::delete_resource(rmanager_s *r) {
	if(r == NULL) {
		return;
	}
	else if(r == _resources) {
		_resources = _resources->s;
	}
	else {
		rmanager_s * tmp = _resources;
		while( tmp->s != r ) tmp = tmp->s;
		tmp->s = r->s;
	}
	free(r->name);
	free(r);
}

// TODO
uint8_t Rmanager::request_resource(Message &in, Message &out) {
	uint8_t res(0);
	/*
	switch(in.get_code()) { // TODO : not sure we can do this way
		case COAP_REQ_DELETE :
			delete_resource();
			break;
		default :
			return exec_request(in, out);
	}
	*/
	return res;
}

// TODO
rmanager_s * Rmanager::get_resource_instance(Message &in) {
	return NULL;
}

uint8_t Rmanager::exec_request(Message &in, Message &out) {
	rmanager_s * tmp = get_resource_instance(in);
	if(tmp == NULL)
		return 1;
	return (*tmp->h)(in, out);
}

void Rmanager::reset() {
	while(_resources != NULL) {
		delete_resource(_resources);
	}
}
