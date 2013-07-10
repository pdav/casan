#ifndef SOS_MSG_H
#define SOS_MSG_H

#include <list>
#include <chrono>

#include "coap.h"
#include "option.h"

namespace sos {

class slave ;
class waiter ;

/*
 * Message encoder/decoder
 *
 * This class hides protocol encoding details
 */

class msg
{
    public:
	// types
	typedef enum msgtype { MT_CON=0, MT_NON, MT_ACK, MT_RST } msgtype_t ;
	typedef enum msgcode { MC_EMPTY=0,
		    MC_GET=1, MC_POST, MC_PUT, MC_DELETE } msgcode_t ;
	// SOS_HELLO is never used
	typedef enum sostype { SOS_NONE=0, SOS_REGISTER, SOS_ASSOC_REQUEST,
		    SOS_ASSOC_ANSWER, SOS_HELLO, SOS_UNKNOWN} sostype_t ;

	msg () ;			// constructor
	msg (const msg &m) ;		// copy constructor
	msg &operator= (const msg &m) ;	// copy assignment constructor
	~msg () ;			// destructor

	// operators
	int operator == (msg &) ;	// only for received messages

	friend std::ostream& operator<< (std::ostream &os, const msg &m) ;

	// basic operations
	int send (void) ;
	l2addr *recv (l2net *l2) ;	// returned addr must be freed by caller

	// mutators (to send messages)
	void peer (slave *s) ;
	void token (void *token, int len) ;
	void id (int id) ;
	void type (msgtype_t type) ;
	void code (int code) ;
	void payload (void *data, int len) ;
	void pushoption (option &o) ;
	void wt (waiter *w) ;

	void stop_retransmit (void) ;	// no need for more retransmits

	// accessors (for received messages)
	slave *peer (void) ;
	pktype_t pktype (void) ;
	void *token (int *toklen) ;
	int id (void) ;
	msgtype_t type (void) ;
	int code (void) ;
	void *payload (int *paylen) ;
	option popoption (void) ;
	msg *reqrep (void) ;
	waiter *wt (void) ;

	void option_reset_iterator (void) ;
	option *option_next (void) ;

	void link_reqrep (msg *m) ;	// m == 0 <=> unlink

	// control messages
	sostype_t sos_type (void) ;
	slaveid_t is_sos_discover (void) ;	// SOS control message
	bool is_sos_associate (void) ;		// SOS control message
	void add_path_ctl (void) ;
	void mk_ctl_hello (long int hid) ;	// options for ctl mssage
	void mk_ctl_assoc (sostimer_t ttl) ;	// options for ctl mssage

    protected:
	timepoint_t expire_ ;		// all msg
	int ntrans_ = 0 ;		// # of transmissions (CON/NON)
	duration_t timeout_ ;		// current timeout (CON)
	timepoint_t next_timeout_ ;	// (CON)

	friend class sos ;

    private:
	waiter *waiter_ = nullptr ;	// wakeup when an answer is received

	// Formatted message, as it appears on the cable/over the air
	byte *msg_ = nullptr ;		// NULL when reset
	int msglen_ = 0 ;		// len of msg
	pktype_t pktype_ = PK_NONE ;

	// Peer
	slave *peer_ = nullptr ;	// if found associated slave

	// CoAP specific variables
	byte *payload_ = nullptr ;	// nul added at the end
	int paylen_ = 0 ;
	byte token_ [COAP_MAX_TOKLEN] ;
	int toklen_ = 0 ;
	msgtype_t type_ = MT_RST ;
	int code_ = MC_EMPTY ;
	int id_ = 0 ;			// message id
	std::list <option> optlist_ ;	// list of all options
	std::list <option>::iterator optiter_ ;

	msg *reqrep_ = nullptr ;	// "request of" or "response of"
	sostype_t sostype_ = SOS_UNKNOWN ;

	static int global_message_id ;

	void coap_encode (void) ;
	bool coap_decode (void) ;

	bool is_sos_ctl_msg (void) ;
} ;

}					// end of namespace sos
#endif
