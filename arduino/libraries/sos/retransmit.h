#ifndef __RETRANSMIT_H__
#define __RETRANSMIT_H__

#include "coap.h"
#include "message.h"

typedef struct _retransmit_s {
	Message *m;
	int time;
	uint8_t nb_retransmissions;
	_retransmit_s *s;	// next
} retransmit_s;

class Retransmit {
	public:
		Retransmit(Coap *c);
		~Retransmit(void);

		void add(Message *m, int time_first_transmit);
		void del(Message *m);
		void del(retransmit_s *r);
		void loop_retransmit(void); // TODO
		void reset(void);

	private:
		retransmit_s * get_retransmit(Message *m);
		retransmit_s * _retransmit;
		Coap  *_c;
};

#endif
