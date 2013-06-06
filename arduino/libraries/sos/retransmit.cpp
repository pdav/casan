#include "retransmit.h"

Retransmit::Retransmit(Coap *c, l2addr **master) 
{
	_c = c;
	_master_addr = master;
	_retransmit = NULL;
}

Retransmit::~Retransmit() 
{
	reset();
}

void Retransmit::add(Message *m, ulong_t time_first_transmit) 
{
	retransmit_s * n = (retransmit_s *) malloc(sizeof(retransmit_s));
	n->m = m ;
	// next retransmit
	n->timel = time_first_transmit;
	n->timen = time_first_transmit + ALEA(ACK_TIMEOUT * ACK_RANDOM_FACTOR);
	n->nb = 0;
	n->s = _retransmit;
	_retransmit = n;
}

void Retransmit::del(Message *m) 
{
	del(get_retransmit(m));
}

void Retransmit::del(retransmit_s *r) 
{
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

retransmit_s * Retransmit::get_retransmit(Message *m) 
{
	retransmit_s *tmp = _retransmit;
	while(tmp != NULL) {
		// TODO : maybe check the token too
		if(tmp->m->get_id() == m->get_id())
			return tmp;
		tmp = tmp->s;
	}
	return NULL;
}

// TODO
void Retransmit::loop_retransmit(void) 
{
	//enum coap_message_type coap_mes_type = in.get_coap_type();
	Serial.println(F("retransmit loop"));

	retransmit_s * tmp = _retransmit;
	while(tmp != NULL) {
		if( tmp->nb >= MAX_RETRANSMIT ) {
			retransmit_s * tmp2 = tmp->s;
			del(tmp);
			tmp = tmp2;
			continue;
		}
		else if( tmp->timen < millis()) {
			_c->send(**_master_addr, *(tmp->m));
			tmp->nb++;
			ulong_t time_tmp = tmp->timen;
			tmp->timen = tmp->timen + 2 * (tmp->timen - tmp->timel);
			tmp->timel = time_tmp;
		}
		tmp = tmp->s;
	}
}

void Retransmit::reset(void) 
{
	while(_retransmit != NULL)
		del(_retransmit);
}

void Retransmit::check_msg_received(Message &in) 
{
	switch(in.get_type())
	{
		case COAP_TYPE_ACK :
			del(&in);
			break;
		default :
			break;
	}
}

void Retransmit::check_msg_sent(Message &in) 
{
	switch(in.get_type())
	{
		case COAP_TYPE_CON :
			add(&in, (ulong_t) millis);
			break;
		default :
			break;
	}
}

