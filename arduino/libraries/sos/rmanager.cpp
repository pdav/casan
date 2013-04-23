#include "rmanager.h"

Rmanager::Rmanager() { 
}

Rmanager::~Rmanager() { 
	rmanager_s * tmp = _resources;
	rmanager_s * tmp2 = tmp;
	while(tmp != NULL) {
		if(tmp->s == NULL) {
			free(tmp->name);
			free(tmp);
			break;
		}
		while(tmp->s != NULL) {
			tmp2 = tmp;
			tmp = tmp->s;
		}
		free(tmp->name);
		free(tmp);
		tmp2->s = NULL;
		tmp = _resources;
		tmp2 = tmp;
	}
}

void Rmanager::add_resource(char *name, uint8_t (*handler)(uint8_t, uint8_t*)) {
	rmanager_s * newresource = malloc(sizeof(rmanager_s));
	newresource->name = malloc(strlen(name) + 1);
	memcpy(newresource->name, name, strlen(name) + 1);
	newresource->h = handler;
	newresource->s = _resources;
	_resources = newresource;
}

void Rmanager::delete_resource(char *name) {
	delete_resource(get_resource_instance(name));
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

uint8_t Rmanager::request_resource(char *name, enum coap_request_method coap_req, uint8_t *option) {
	switch(coap_req) {
		case COAP_REQ_DELETE :
			delete_resource(name);
			break;
		default :
			return exec_request(name, coap_req, option);
	}

	return res;
}

rmanager_s * Rmanager::get_resource_instance(char *name) {
	rmanager_s * tmp = _resources;
	while(tmp != NULL) {
		if(strcmp(tmp->name, name) == 0)
			return tmp;
		tmp = tmp->s;
	}
	return NULL;
}

uint8_t Rmanager::exec_request(char *name, enum coap_request_method coap_req, uint8_t *option) {
	rmanager_s * tmp = get_resource_instance(name);
	if(tmp == NULL)
		return 1;
	return (*tmp->h)(coap_req, option);
}

void Rmanager::reset() {
	while(_resources != NULL) {
		delete_resource(_resources);
	}
}
