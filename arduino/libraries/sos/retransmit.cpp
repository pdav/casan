#include "retransmit.h"

Retransmit::Retransmit(Coap *c) {
	_c = c;
}

Retransmit::~Retransmit() {
	reset();
}

void Retransmit::add(Message *m, int time_first_transmit) {
	retransmit_s * n = (retransmit_s *) malloc(sizeof(retransmit_s));
	n->m = m ;
	n->time = time_first_transmit;
	n->nb_retransmissions = 0;
	n->s = _retransmit;
	_retransmit = n;
}

void Retransmit::del(Message *m) {
	del(get_retransmit(m));
}

void Retransmit::del(retransmit_s *r) {
	if(r == NULL) {
		return;
	}
	else if(r == _retransmit) {
		_retransmit = _retransmit->s;
	}
	else {
		retransmit_s * tmp = _retransmit;
		while( tmp->s != r ) tmp = tmp->s;
		tmp->s = r->s;
	}
	delete r->m;
	free(r);
}

retransmit_s * Retransmit::get_retransmit(Message *m) {
	retransmit_s *tmp = _retransmit;
	while(tmp != NULL) {
		if(tmp->m->get_id() == m->get_id())
			return tmp;
		tmp = tmp->s;
	}
	return NULL;
}

void Retransmit::loop_retransmit(void) {
}

void Retransmit::reset(void) {
	while(_retransmit != NULL)
		del(_retransmit);
}
