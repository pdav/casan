#ifndef SOS_MSG_H
#define SOS_MSG_H

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

	msg () ;				// constructor
	~msg () ;				// destructor

	// basic operations
	void send (void) ;
	void recv (l2net *l2) ;

	// mutators (to send messages)
	void peer (slave *s) ;
	void token (void *token, int len) ;
	void type (msgtype_t type) ;
	void code (int code) ;
	void payload (void *data, int len) ;
	void handler (reply_handler_t h) ;

	// accessors (for received messages)
	slave *peer (void) ;
	pktype_t pktype (void) ;
	void *token (int *toklen) ;
	int id (void) ;
	msgtype_t type (void) ;
	bool isanswer (void) ;
	bool isrequest (void) ;
	int code (void) ;
	void *payload (int *paylen) ;

    protected:
	// CoAP protocol parameters
	int timeout_ ;				// current timeout
	int ntrans_ ;				// number of transmissions

	reply_handler_t handler_ ;

	friend class engine ;

    private:
	// Formatted message, as it appears on the cable/over the air
	byte *msg_ ;				// NULL when reset
	int msglen_ ;				// len of msg
	pktype_t pktype_ ;

	// CoAP specific variables
	slave *peer_ ;
	unsigned char *payload_ ;
	int paylen_ ;
	unsigned char *token_ ;
	int toklen_ ;
	msgtype_t type_ ;
	int code_ ;
	int id_ ;				// message id

	static int global_message_id ;

} ;

#endif
