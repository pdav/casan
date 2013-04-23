#ifndef SOS_MSG_H
#define SOS_MSG_H

#include "sos.h"
#include "l2.h"

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
	void reset () ;				// reset to a known state
	void send (l2net *l2) ;
	void recv (l2net *l2) ;

	// settors (to send messages)
	void destaddr (l2addr *a) ;
	void token (int token) ;
	void type (msgtype_t type) ;
	void code (int code) ;
	void payload (void *data, int len) ;

	// accessor (for received messages)
	l2addr *srcaddr (void) ;
	pktype_t pktype (void) ;
	int token (void) ;
	int id (void) ;
	msgtype_t type (void) ;
	isanswer (void) ;
	isrequest (void) ;
	int code (void) ;
	int payload (void *data) ;

    private:
	unsigned char *msg ;			// NULL when reset
	int len ;				// len of msg

	unsigned char *payload ;
	int token_ ;
	int id ;				// message id
	req_handler_t handler_ ;
	enum internal_req_status status ;

	static int global_message_id ;

	friend class engine ;
} ;

#endif
