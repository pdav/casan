#include "retransmit.h"

Retransmit::Retransmit(Coap *c) {
	_c = c;
}
Retransmit::~Retransmit() {
	reset();
}

void Retransmit::add(Message *m, int time_first_transmit) {
	retransmit_s * n = malloc(sizeof(retransmit_s));
	n->m = m ;
	n->time = time_first_transmit;
	n->nb_retransmissions(0);
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
	free(r->name);
	free(r);
}

retransmit_s * Retransmit::get_retransmit(Message *m) {
	retransmit_s *tmp = _retransmit;
	uint8_t *tmp_id0;
	uint8_t	*tmp_id1;
	while(tmp != NULL) {
		tmp_id0 = tmp->m.get_id();
		tmp_id1 = m->get_id();
		if(tmp_id0[0] == tmp_id1[0] && tmp_id0[1] == tmp_id1[1])
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
