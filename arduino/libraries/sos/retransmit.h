#ifndef __RETRANSMIT_H__
#define __RETRANSMIT_H__

#include "util.h"
#include "defs.h"
#include "enum.h"
#include "coap.h"
#include "message.h"
#include "l2eth.h"

#define DEFAULT_TIMER 4000

typedef struct _retransmit_s {
	Message *m;
	ulong_t timel;	// time last transmition
	ulong_t timen;	// time next transmition
	uint8_t nb; // nb retransmit. 
	_retransmit_s *s;	// next
} retransmit_s;

class Retransmit {
	public:
		Retransmit(Coap *c, l2addr **master);
		~Retransmit(void);

		void add(Message *m, ulong_t time_first_transmit);
		void del(Message *m);
		void del(retransmit_s *r);
		void loop_retransmit(void); // TODO
		void reset(void);
		void check_message(Message &in);

	private:
		retransmit_s * get_retransmit(Message *m);
		retransmit_s * _retransmit;
		Coap  *_c;
		l2addr ** _master_addr;
};

#endif
