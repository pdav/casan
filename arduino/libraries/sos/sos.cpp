#include "sos.h"

#define	SOS_NAMESPACE1		".well-known"
#define	SOS_NAMESPACE2		"sos"
#define	SOS_HELLO		"hello=%ld"
#define	SOS_REGISTER		"slave=%ld"
#define	SOS_ASSOC		"assoc=%ld"
#define SOS_ASSOC_RETURN_PAYLOAD	"<resource list>"

static struct
{
	const char *path ;
	int len ;
}
sos_namespace [] =
{
	{  SOS_NAMESPACE1, sizeof SOS_NAMESPACE1 - 1 },
	{  SOS_NAMESPACE2, sizeof SOS_NAMESPACE2 - 1 },
} ;

extern l2addr_eth l2addr_eth_broadcast ;

// TODO : set all variables
	Sos::Sos(l2addr *mac_addr, long int uuid) 
: _nttl(SOS_DEFAULT_TTL), _status(SL_COLDSTART), _uuid(uuid) 
{
	_master_addr = NULL;
	_current_message_id = 1;

	_eth = new EthernetRaw();
	_eth->set_master_addr(_master_addr);
	_eth->set_mac(mac_addr);
	_eth->set_ethtype((int) SOS_ETH_TYPE);

	_coap = new Coap(_eth);
	_rmanager = new Rmanager();
	_retransmition_handler = new Retransmit(_coap, &_master_addr);
	_status = SL_COLDSTART;
}

void Sos::set_master_addr(l2addr *a) 
{
	if(_master_addr != NULL)
		delete _master_addr;
	_master_addr = a;
	_eth->set_master_addr(_master_addr);
}

void Sos::register_resource(char *name, int namelen,
		uint8_t (*handler)(Message &in, Message &out) ) 
{
	_rmanager->add_resource(name, namelen, handler);
}

// TODO : we need to restart all the application, 
// delete all the history of the exchanges
void Sos::reset (void) 
{
	_status = SL_COLDSTART;
	_current_message_id = 0;
	_nttl = SOS_DEFAULT_TTL;
	_rmanager->reset();
	_retransmition_handler->reset();
	// This have to be the only time we delete the instance of master addr
	delete _master_addr;
	_master_addr = NULL;
}

void Sos::loop() 
{
	// to check all retrans. we have to do

	_retransmition_handler->loop_retransmit(); 
	Message in;
	Message out;
	eth_recv_t ret;

	switch(_status) {

		case SL_COLDSTART :
			Serial.println(F("SL_COLDSTART"));
			delay(_time_to_wait);
			mk_registration();

			// compute the next waiting time laps for a message
			_time_to_wait = 
				(_time_to_wait + SOS_DELAY_INCR) < SOS_DELAY_MAX ? 
				_time_to_wait + SOS_DELAY_INCR : _time_to_wait ;
			//Serial.print(F("time to wait : "));
			//Serial.println(_time_to_wait);

			_status = SL_WAITING_UNKNOWN;

		case SL_WAITING_UNKNOWN :
			Serial.println(F("\033[33mSL_WAITING_UNKNOWN\033[00m"));

			while(  _status == SL_WAITING_UNKNOWN &&
					(ret = _coap->recv(in)) != ETH_RECV_EMPTY)
			{
				if(ret == ETH_RECV_RECV_OK) 
				{
					_retransmition_handler->check_msg_received(in); 
					if(is_hello(in)) 
					{
						if(_master_addr)
							delete _master_addr;
						_master_addr = new l2addr_eth();
						_coap->get_mac_src(_master_addr);
						_current_message_id = in.get_id() + 1;
						_status = SL_WAITING_KNOWN;

						//((l2addr_eth *) _master_addr)->print();
						_time_to_wait = SOS_DELAY;

					} 
					else if (is_associate(in)) 
					{
						if(_master_addr)
							delete _master_addr;
						_master_addr = new l2addr_eth();
						_coap->get_mac_src(_master_addr);
						_current_message_id = in.get_id() + 1;
						_status = SL_RUNNING;

						PRINT_DEBUG_STATIC("ASSOCIATE IN UNKNOWN");

						mk_ack_assoc(in, out);
						out.print();
						_coap->send(*_master_addr, out);
						_time_to_wait = SOS_DELAY;
					} 
					else 
					{
						Serial.println(F("\033[31mrecv error\033[00m"));
					}
				}
			}
			mk_registration();

			// compute the next waiting time laps for a message
			_time_to_wait = 
				(_time_to_wait + SOS_DELAY_INCR) < SOS_DELAY_MAX ? 
				_time_to_wait + SOS_DELAY_INCR : _time_to_wait ;
			//Serial.print(F("time to wait : "));
			//Serial.println(_time_to_wait);
			delay(_time_to_wait);
			break;

		case SL_WAITING_KNOWN :
			Serial.println(F("\033[33mSL_WAITING_KNOWN\033[00m"));
			PRINT_DEBUG_STATIC("MSG RECEIVED");
			while(  _status == SL_WAITING_KNOWN &&
					(ret = _coap->recv(in)) != ETH_RECV_EMPTY)
			{
				if(ret == ETH_RECV_RECV_OK) 
				{
					_retransmition_handler->check_msg_received(in); 

					if(is_hello(in)) 
					{
						if(_master_addr)
							delete _master_addr;
						_master_addr = new l2addr_eth();
						_coap->get_mac_src(_master_addr);
						_current_message_id = in.get_id() + 1;
						_time_to_wait = SOS_DELAY;

					} 
					else if (is_associate(in)) 
					{
						if(_master_addr)
							delete _master_addr;
						_master_addr = new l2addr_eth();
						_coap->get_mac_src(_master_addr);
						_current_message_id = in.get_id() + 1;
						_status = SL_RUNNING ;

						mk_ack_assoc(in, out);
						out.print();
						_coap->send(*_master_addr, out);
						_time_to_wait = SOS_DELAY;
					} 
					else 
					{
						Serial.println(F("\033[31mrecv error\033[00m"));
					}
				}
			}

			_time_to_wait = 
				(_time_to_wait + SOS_DELAY_INCR) < SOS_DELAY_MAX ? 
				_time_to_wait + SOS_DELAY_INCR : _time_to_wait ;
			//Serial.print(F("time to wait : "));
			//Serial.println(_time_to_wait);
			delay(_time_to_wait);
			mk_registration();

			// like SL_WAITING_UNKNOWN we do not take into account
			// other messages than ALLOC & HELLO
			break;

		case SL_RENEW :		// TODO
			Serial.println(F("\033[33mSL_RENEW\033[00m"));
			// TODO send message register & check the timer
			// at TTL = 0 : 
			// w. unknown & wait SOS_DELAY then broadcast a registration
			// if reception HELLO w. known then directed registration

			_time_to_wait = 
				(_time_to_wait + SOS_DELAY_INCR) < SOS_DELAY_MAX ? 
				_time_to_wait + SOS_DELAY_INCR : _time_to_wait ;
			//Serial.print(F("time to wait : "));
			//Serial.println(_time_to_wait);
			delay(_time_to_wait);
			break;

		case SL_RUNNING :	// TODO
			Serial.println(F("\033[33mSL_RUNNING\033[00m"));

			PRINT_DEBUG_STATIC("MSG RECEIVED");
			while(  _status == SL_RUNNING &&
					(ret = _coap->recv(in)) != ETH_RECV_EMPTY)
			{
				if(ret == ETH_RECV_RECV_OK) 
				{
					_retransmition_handler->check_msg_received(in); 

					if(is_hello(in)) 
					{
						if(_master_addr)
							delete _master_addr;
						_master_addr = new l2addr_eth();
						_coap->get_mac_src(_master_addr);
						_current_message_id = in.get_id() + 1;
						_status = SL_WAITING_KNOWN;
						_time_to_wait = SOS_DELAY;
					} 
					else if (is_associate(in)) 
					{
						if(_master_addr)
							delete _master_addr;
						_master_addr = new l2addr_eth();
						_coap->get_mac_src(_master_addr);
						_current_message_id = in.get_id() + 1;
						_status = SL_RUNNING;

						mk_ack_assoc(in, out);
						out.print();
						_coap->send(*_master_addr, out);

						_time_to_wait = SOS_DELAY;
					} 
					else 
					{
						Serial.println(F("\033[31mrecv error\033[00m"));
					}
					//_ttl = 
				}
			}


			//	deduplicate();
			_rmanager->request_resource(in, out);

			break;

		default :
			Serial.println(F("Error : sos status not known"));
			PRINT_DEBUG_DYNAMIC(_status);
			break;
	}
	delay(_time_to_wait);
}

void Sos::mk_assoc(Message &out)
{
	mk_ctl_msg(out);
	{
		char message[SOS_BUF_LEN];
		snprintf(message, SOS_BUF_LEN, SOS_ASSOC, _uuid);
		option o (option::MO_Uri_Query,
				(unsigned char*) message,
				strlen(message)) ;
		out.push_option (o) ;
	}
}

void Sos::mk_ack_assoc(Message &in, Message &out)
{
	// send an ack back
	out.set_type(COAP_TYPE_ACK);
	out.set_code(COAP_RETURN_CODE(2,5));
	out.set_id(in.get_id());
	mk_ctl_msg(out);
	out.set_payload(
			sizeof SOS_ASSOC_RETURN_PAYLOAD -1,
			(uint8_t *) SOS_ASSOC_RETURN_PAYLOAD 
			);

	//_rmanager->get_all_resources(out);
}

/******************************************************************************
 * SOS control messages
 */

bool Sos::is_ctl_msg (Message &m)
{
	int i = 0;

	m.reset_get_option();
	for(option * o = m.get_option() ; o != NULL ; o = m.get_option()) {
		if (o->optcode() == option::MO_Uri_Path)
		{
			if (i >= NTAB (sos_namespace))
				return false ;
			if (sos_namespace [i].len != o->optlen())
				return false ;
			if (memcmp (sos_namespace [i].path, (const void *)o->val(), o->optlen()))
				return false ;
			i++ ;
		}
	}
	m.reset_get_option();
	if (i != NTAB (sos_namespace))
		return false ;
	return true ;
}

void Sos::mk_ctl_msg (Message &m)
{
	int i = 0;
	option path1 (option::MO_Uri_Path,
			(void*)sos_namespace[0].path,
			sos_namespace[0].len) ;
	option path2 (option::MO_Uri_Path,
			(void*)sos_namespace[1].path,
			sos_namespace[1].len) ;

	m.push_option(path1);
	m.push_option(path2);
}

void Sos::mk_registration() 
{
	Serial.println(F("registration"));
	Message m;
	l2addr * dest = (_master_addr == NULL) ? &l2addr_eth_broadcast : _master_addr;

	m.set_id(_current_message_id);
	m.set_type( COAP_TYPE_NON );
	m.set_code( COAP_CODE_POST );
	mk_ctl_msg(m);

	{
		char message[SOS_BUF_LEN];
		snprintf(message, SOS_BUF_LEN, SOS_REGISTER, _uuid);
		option o (option::MO_Uri_Query,
				(unsigned char*) message,
				sizeof SOS_REGISTER -1) ;
		m.push_option (o) ;
	}
	_coap->send(*dest, m);
}

bool Sos::is_associate (Message &m)
{
	bool found = false ;

	m.reset_get_option();
	if (	m.get_type() == COAP_TYPE_CON && 
			m.get_code() == COAP_CODE_POST && 
			is_ctl_msg (m))
	{
		for(option * o = m.get_option() ; o != NULL ; o = m.get_option()) 
		{
			if (o->optcode() == option::MO_Uri_Query)
			{
				long int n ;

				// we benefit from the added nul byte at the end of val
				if (sscanf ((const char *) o->val(), SOS_ASSOC, &n) == 1)
				{
					_nttl = n;
					PRINT_DEBUG_STATIC("NTTL : ");
					PRINT_DEBUG_DYNAMIC(_nttl);
					found = true ;
					// continue, just in case there are other query strings
				}
				else
				{
					found = false ;
					break ;
				}
			}
		}
		if (found)
			_status = SL_RUNNING ;
	}
	return found ;
}

// check if a hello msg is received & new
bool Sos::is_hello(Message &m)
{
	bool found = false ;

	if (	m.get_type() == COAP_TYPE_NON && 
			m.get_code() == COAP_CODE_POST && 
			is_ctl_msg (m))
	{

		m.reset_get_option();
		for(option * o = m.get_option() ; o != NULL ; o = m.get_option()) 
		{
			if (o->optcode() == option::MO_Uri_Query)
			{
				long int n ;

				// we benefit from the added nul byte at the end of val
				if (sscanf ((const char *) o->val(), SOS_HELLO, &n) == 1)
				{
					if(_hlid != n)
					{
						_hlid = n;
						found = true ;
					}
				}
			}
		}
	}
	return found;
}

void check_msg(Message &in, Message &out)
{
}

