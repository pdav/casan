#ifndef SOS_REQ_H
#define SOS_REQ_H

#include "l2.h"

class request ;

typedef enum req_status { REQ_SUCCESS, REQ_FAIL } req_status_t ;
typedef void (*req_handler_t) (request *r, req_status_t reqstatus) ;

enum internal_req_status {
    REQ_NONE,				// request not registerd
    REQ_WAITING,			// request waiting for an answer
} ;

class request
{
    public:
	request () ;				// constructor
	~request () ;				// destructor

	void dest (l2net  *l2, l2addr *a) ;
	// timeout (int t) ;
	// token (int tok) ;
	void handler (req_handler_t func) ;

    protected:
	l2addr *daddr ;
	l2net  *dl2 ;

	// int timeout_ ;
	// int token_ ;
	int id ;				// message id
	req_handler_t handler_ ;
	enum internal_req_status status ;

	static int global_message_id ;

	friend class engine ;
} ;

#endif
