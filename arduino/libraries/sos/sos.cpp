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
	_master_addr = new l2addr_eth(l2addr_eth_broadcast);
	reset_master();	// _master_addr becomes a broadcast

	_eth->set_master_addr(_master_addr);
	_current_message_id = 1;

	_eth = new EthernetRaw();
	_eth->set_mac(mac_addr);
	_eth->set_ethtype((int) SOS_ETH_TYPE);

	_coap = new Coap(_eth);
	_rmanager = new Rmanager();
	_retransmition_handler = new Retransmit(_coap, &_master_addr);
	_status = SL_COLDSTART;

	current_time.cur();
}

void Sos::reset_master(void) 
{

	if(_master_addr)
	{
		PRINT_DEBUG_STATIC("OLD ADDR");
		((l2addr_eth *) _master_addr)->print();
	}
	else
	{
		PRINT_DEBUG_STATIC("\033[31mERROR : master not set yet ! \033[00m");
	}

	PRINT_DEBUG_STATIC("\033[36mset master addr to broadcast\033[00m");
	*((l2addr_eth *) _master_addr) = l2addr_eth_broadcast;
	((l2addr_eth *) _master_addr)->print();
	_hlid = 0;
}

void Sos::register_resource(
		char *name, int namelen,
		char *title, int titlelen,
		char *rt, int rtlen,
		uint8_t (*handler)(Message &in, Message &out) ) 
{
	_rmanager->add_resource(
			name, namelen, 
			title, titlelen, 
			rt, rtlen, 
			handler);
}

// TODO : we need to restart all the application, 
// delete all the history of the exchanges
void Sos::reset (void) 
{
	_status = SL_COLDSTART;
	_current_message_id = 1;
	_nttl = SOS_DEFAULT_TTL;
	_rmanager->reset();
	_retransmition_handler->reset();

	reset_master();
}

void Sos::loop() 
{
	// to check all retrans. we have to do

	_retransmition_handler->loop_retransmit(); 
	Message in;
	Message out;
	eth_recv_t ret;

	current_time.cur();

	switch(_status) 
	{

		case SL_COLDSTART :
			Serial.println(F("SL_COLDSTART"));
			mk_registration();

			_status = SL_WAITING_UNKNOWN;
			// the next waiting time laps for a message
			_next_time_increment = SOS_DELAY;

			// next time we will send a register msg
			_next_register = current_time;
			_next_register.add(_next_time_increment);

		case SL_WAITING_UNKNOWN :
			Serial.println(F("\033[33mSL_WAITING_UNKNOWN\033[00m"));

			while(  _status == SL_WAITING_UNKNOWN &&
					(ret = _coap->recv(in)) != ETH_RECV_EMPTY)
			{

				//print_coap_ret_type(ret);

				if(ret == ETH_RECV_RECV_OK) 
				{

					_retransmition_handler->check_msg_received(in); 

					// we only take ctl msg into account
					if(is_ctl_msg(in))
					{

						if(is_hello(in)) 
						{

							_coap->get_mac_src(_master_addr);
							_current_message_id = in.get_id() + 1;
							_status = SL_WAITING_KNOWN;

							//((l2addr_eth *) _master_addr)->print();
							_next_time_increment = SOS_DELAY;
							_next_register = current_time;
							_next_register.add(_next_time_increment);

						} 
						else if (is_associate(in)) 
						{
							mk_ack_assoc(in, out);
						} 
						else 
						{
							Serial.println(F("\033[31mctl msg but not took into account\033[00m"));
						}

					}

				}

			}

			if( _next_register < current_time)
			{
				mk_registration();

				// compute the next waiting time laps for a message
				_next_time_increment = 
					(_next_time_increment + SOS_DELAY_INCR) > SOS_DELAY_MAX ? 
					SOS_DELAY_MAX : _next_time_increment + SOS_DELAY_INCR;
				_next_register = current_time;
				_next_register.add(_next_time_increment);
			}

			break;

		case SL_WAITING_KNOWN :
			Serial.println(F("\033[33mSL_WAITING_KNOWN\033[00m"));

			// like SL_WAITING_UNKNOWN we do not take into account
			// other messages than ASSOC & HELLO

			while(  _status == SL_WAITING_KNOWN &&
					(ret = _coap->recv(in)) != ETH_RECV_EMPTY)
			{

				if(ret == ETH_RECV_RECV_OK) 
				{
					_retransmition_handler->check_msg_received(in); 

					if(is_ctl_msg(in))
					{
						if(is_hello(in)) 
						{
							_coap->get_mac_src(_master_addr);
							_current_message_id = in.get_id() + 1;

							_next_time_increment = SOS_DELAY;
							_next_register = current_time;
							_next_register.add(_next_time_increment);

						} 
						else if (is_associate(in)) 
						{

							mk_ack_assoc(in, out);

						} 
						else 
						{
							Serial.println(F("\033[31mctl msg but not took into account\033[00m"));
						}
					}

				} // if

			} // while

			// it is possible to don't be in waiting known status
			if(_status == SL_WAITING_KNOWN)
			{
				// if we reach the max increment and the time is over
				// we go back to the waiting unknown status
				if( _next_time_increment == SOS_DELAY_MAX && 
						time::diff(_next_register, current_time) <= 0)
				{
					reset_master();	// _master_addr becomes a broadcast

					_next_time_increment = SOS_DELAY;
					_next_register = current_time;
					_next_register.add(_next_time_increment);

					_status = SL_WAITING_UNKNOWN;
				}

				if( _next_register < current_time)
				{

					mk_registration();

					_next_time_increment = 
						(_next_time_increment + SOS_DELAY_INCR) > SOS_DELAY_MAX ? 
						SOS_DELAY_MAX : _next_time_increment + SOS_DELAY_INCR;
					_next_register = current_time;
					_next_register.add(_next_time_increment);

				}

			}

			break;

		case SL_RENEW :
			Serial.println(F("\033[33mSL_RENEW\033[00m"));

			while(  _status == SL_RENEW &&
					(ret = _coap->recv(in)) != ETH_RECV_EMPTY)
			{

				if(ret == ETH_RECV_RECV_OK) 
				{
					_retransmition_handler->check_msg_received(in); 

					if(is_ctl_msg(in))
					{
						if(is_hello(in)) 
						{

							_coap->get_mac_src(_master_addr);
							_current_message_id = in.get_id() + 1;
							_status = SL_WAITING_KNOWN;

							_next_time_increment = SOS_DELAY;
							_next_register = current_time;
							_next_register.add(_next_time_increment);
							

							mk_registration();
						} 
						else if (is_associate(in)) 
						{
							mk_ack_assoc(in, out);
						} 
						else 
						{
							Serial.println(F("\033[31mctl msg but not took into account\033[00m"));
						}
					}
				}

			}

			// it is possible to don't be in renew status
			if(_status == SL_RENEW)
			{
				if( _next_register < current_time)
				{

					_next_time_increment = 
						(_next_time_increment + SOS_DELAY_INCR) > SOS_DELAY_MAX ? 
						SOS_DELAY_MAX : _next_time_increment + SOS_DELAY_INCR;

					_next_register = current_time;
					_next_register.add(_next_time_increment);

					mk_registration();

				}

				// we test if we have to go back in waiting unknown status 
				// = ttl timeout
				if(time::diff(_ttl_timeout, current_time) <= 0)
				{

					reset_master();	// _master_addr becomes a broadcast

					_status = SL_WAITING_UNKNOWN;
					_next_time_increment = SOS_DELAY;
					mk_registration();
				}
			}

			break;

		case SL_RUNNING :	// TODO
			Serial.println(F("\033[33mSL_RUNNING\033[00m"));

			while(  _status == SL_RUNNING &&
					(ret = _coap->recv(in)) != ETH_RECV_EMPTY)
			{

				if(ret == ETH_RECV_RECV_OK) 
				{
					_retransmition_handler->check_msg_received(in); 

					if(is_ctl_msg(in))
					{
						if(is_hello(in)) 
						{

							_coap->get_mac_src(_master_addr);
							_current_message_id = in.get_id() + 1;
							_status = SL_WAITING_KNOWN;

							_next_time_increment = SOS_DELAY;
							_next_register = current_time;
							_next_register.add(_next_time_increment);

						}
						else if (is_associate(in)) 
						{
							mk_ack_assoc(in, out);
						} 
						else 
						{
							Serial.println(F("\033[31mctl msg but not took into account\033[00m"));
						}
					}
					else
					{
						uint8_t ret = _rmanager->request_resource(in, out);
						if(ret > 0)
						{
							Serial.println(F("\033[31mThere is a problem with the request\033[00m"));
						}
						else
						{
							Serial.println(F("\033[36mWe sent the answer\033[00m"));
							_coap->send(*_master_addr, out);
						}
					}
				}

			}

			if(_status == SL_RUNNING)
			{

				// if current time <= ttl timeout /2
				if(time::diff(_ttl_timeout_mid, current_time) <= 0)
				{
					mk_registration();
					_next_time_increment = SOS_DELAY;
					_next_register = current_time;
					_next_register.add(_next_time_increment);
					_status = SL_RENEW;
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

	delay(5);
}

void Sos::mk_ack_assoc(Message &in, Message &out)
{
	// we get the new master
	_coap->get_mac_src(_master_addr);

	// the current mid is the next after the id of the received msg
	_current_message_id = in.get_id() + 1;

	// we are now in running mode
	_status = SL_RUNNING;

	// send an ack back
	out.set_type(COAP_TYPE_ACK);
	out.set_code(COAP_RETURN_CODE(2,5));
	out.set_id(in.get_id());
	mk_ctl_msg(out);

	// will get the resources and set them in the payload in the right format
	_rmanager->get_all_resources(out);

	// send the packet
	_coap->send(*_master_addr, out);

	// we received an assoc msg : the timer is renewed
	_next_time_increment = SOS_DELAY;

	current_time.cur();

	// the ttl timeout is set to current time + _nttl
	_ttl_timeout = current_time;
	_ttl_timeout.add(_nttl);

	// there is a mid-term ttl setted for passing in renew status
	_ttl_timeout_mid = current_time ;
	_ttl_timeout_mid.add(_nttl / 2);
}

/**********************
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

	m.set_id(_current_message_id);

	_current_message_id++;

	m.set_type( COAP_TYPE_NON );
	m.set_code( COAP_CODE_POST );
	mk_ctl_msg(m);

	{
		char message[SOS_BUF_LEN];
		snprintf(message, SOS_BUF_LEN, SOS_REGISTER, _uuid);
		option o (option::MO_Uri_Query,
				(unsigned char*) message,
				strlen(message)) ;
		m.push_option (o) ;
	}

	_coap->send(*_master_addr, m);
}

bool Sos::is_associate (Message &m)
{
	bool found = false ;

	m.reset_get_option();
	if (	m.get_type() == COAP_TYPE_CON && 
			m.get_code() == COAP_CODE_POST)
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
					PRINT_DEBUG_STATIC("\033[31m TTL recv \033[00m ");
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
	}

	return found ;
}

// check if a hello msg is received & new
bool Sos::is_hello(Message &m)
{
	bool found = false ;

	// a hello msg is NON POST
	if (	m.get_type() == COAP_TYPE_NON && 
			m.get_code() == COAP_CODE_POST)
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

					// we take into consideration a new hello msg only
					// if there is a new hlid number
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

// maybe for further implementation
void check_msg(Message &in, Message &out)
{
}

/*****************************************
 * useful display of pieces of information
 */

void Sos::print_coap_ret_type(eth_recv_t ret)
{
	switch(ret)
	{
		case ETH_RECV_WRONG_SENDER :
			PRINT_DEBUG_STATIC("ETH_RECV_WRONG_SENDER");
			break;
		case ETH_RECV_WRONG_DEST :
			PRINT_DEBUG_STATIC("ETH_RECV_WRONG_DEST");
			break;
		case ETH_RECV_WRONG_ETHTYPE :
			PRINT_DEBUG_STATIC("ETH_RECV_WRONG_ETHTYPE");
			break ;
		case ETH_RECV_RECV_OK :
			PRINT_DEBUG_STATIC("ETH_RECV_RECV_OK");
			break;
		default :
			PRINT_DEBUG_STATIC("ERROR RECV");
			break;
	}
}
