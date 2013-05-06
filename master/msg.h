#ifndef SOS_MSG_H
#define SOS_MSG_H

#include <list>
#include <chrono>

#include "sos.h"
#include "l2.h"

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

	void complete (void) ;			// no need for more retransmits
	void handler (reply_handler_t h) ;	// for answers to requests

	// accessors (for received messages)
	slave *peer (void) ;
	pktype_t pktype (void) ;
	void *token (int *toklen) ;
	int id (void) ;
	msgtype_t type (void) ;
	int code (void) ;
	void *payload (int *paylen) ;
	msg *reqrep (void) ;

	void link_reqrep (msg *m) ;		// m == 0 <=> unlink
	sostype_t sos_type (void) ;

	slaveid_t is_sos_register (void) ;	// SOS control message
	bool is_sos_associate (void) ;		// SOS control message

    protected:
	std::chrono::system_clock::time_point expire_ ;	// all msg
	int ntrans_ ;				// # of transmissions (CON/NON)
	std::chrono::milliseconds timeout_ ;	// current timeout (CON)
	std::chrono::system_clock::time_point next_timeout_ ;	// (CON)

	reply_handler_t handler_ ;

	friend class engine ;

    private:
	// Formatted message, as it appears on the cable/over the air
	byte *msg_ ;				// NULL when reset
	int msglen_ ;				// len of msg
	pktype_t pktype_ ;

	// Peer
	slave *peer_ ;				// if found associated slave

	// CoAP specific variables
	unsigned char *payload_ ;		// nul added at the end
	int paylen_ ;
	unsigned char *token_ ;
	int toklen_ ;
	msgtype_t type_ ;
	int code_ ;
	int id_ ;				// message id

	msg *reqrep_ ;				// "request of" or "response of"
	sostype_t sostype_ ;

	static int global_message_id ;

	void coap_encode (void) ;
	bool coap_decode (void) ;
} ;

#endif
