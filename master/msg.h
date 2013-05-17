#ifndef SOS_MSG_H
#define SOS_MSG_H

#include <list>
#include <chrono>

#include "defs.h"
#include "sos.h"
#include "l2.h"
#include "option.h"

class msg ;
class slave ;

typedef void (*reply_handler_t) (msg *request, msg *reply) ;

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

	void stop_retransmit (void) ;	// no need for more retransmits
	void handler (reply_handler_t h) ;	// for answers to requests

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

	void link_reqrep (msg *m) ;	// m == 0 <=> unlink

	// control messages
	sostype_t sos_type (void) ;
	slaveid_t is_sos_discover (void) ;	// SOS control message
	bool is_sos_associate (void) ;		// SOS control message
	void add_path_ctl (void) ;
	void mk_ctl_hello (long int hid) ;	// options for ctl mssage
	void mk_ctl_assoc (slavettl_t ttl) ;	// options for ctl mssage

    protected:
	timepoint_t expire_ ;		// all msg
	int ntrans_ ;			// # of transmissions (CON/NON)
	duration_t timeout_ ;		// current timeout (CON)
	timepoint_t next_timeout_ ;	// (CON)

	reply_handler_t handler_ ;

	friend class engine ;

    private:
	// Formatted message, as it appears on the cable/over the air
	byte *msg_ ;			// NULL when reset
	int msglen_ ;			// len of msg
	pktype_t pktype_ ;

	// Peer
	slave *peer_ ;			// if found associated slave

	// CoAP specific variables
	byte *payload_ ;		// nul added at the end
	int paylen_ ;
	byte token_ [COAP_MAX_TOKLEN] ;
	int toklen_ ;
	msgtype_t type_ ;
	int code_ ;
	int id_ ;			// message id
	std::list <option> optlist_ ;	// list of all options

	msg *reqrep_ ;			// "request of" or "response of"
	sostype_t sostype_ ;

	static int global_message_id ;

	void coap_encode (void) ;
	bool coap_decode (void) ;

	bool is_sos_ctl_msg (void) ;
} ;

#endif
