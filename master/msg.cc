#include <iostream>
#include <cstring>

#include "defs.h"
#include "sos.h"
#include "l2.h"
#include "msg.h"
#include "slave.h"
#include "utils.h"

int msg::global_message_id = 1 ;

#define	RESET_BINARY	if (msg_) delete msg_		// no ";"
#define	RESET_POINTERS	if (msg_) delete msg_ ; \
			if (payload_) delete payload_ ; \
			if (token_) delete token_	// no ";"
#define	RESET_VALUES	peer_ = 0 ; \
			msg_ = 0     ; msglen_ = 0 ; \
			payload_ = 0 ; paylen_ = 0 ; \
			token_ = 0   ; toklen_ = 0 ; \
			timeout_ = std::chrono::milliseconds (0) ; \
			ntrans_ = 0 ; \
			id_ = 0				// no ";"

#define	FORMAT_BYTE0(ver,type,toklen) \
			((((unsigned int) (ver) & 0x3) << 6) | \
			 (((unsigned int) (type) & 0x3) << 4) | \
			 (((unsigned int) (toklen) & 0x7)) \
			 )
#define	COAP_VERSION(b)	(((b) [0] >> 30) & 0x3)
#define	COAP_TYPE(b)	(((b) [0] >> 28) & 0x3)
#define	COAP_TOKLEN(b)	(((b) [0] >> 24) & 0xf)
#define	COAP_CODE(b)	(((b) [1]))
#define	COAP_ID(b)	(((b) [2] << 8) | (b) [3])

#define	ALLOC_COPY(f,m,l)	(f=new byte [(l)], std::memcpy (f, (m), (l)))


msg::msg ()
{
    RESET_VALUES ;
}

msg::msg (const msg &m)
{
    *this = m ;
    if (msg_)
	ALLOC_COPY (msg_, m.msg_, msglen_) ;
    if (token_)
	ALLOC_COPY (token_, m.token_, toklen_) ;
    if (payload_)
	ALLOC_COPY (payload_, m.payload_, paylen_) ;
}

msg::~msg ()
{
    RESET_POINTERS ;
}


/******************************************************************************
 * Operators
 */

// Only for received messages
int msg::operator == (msg &m)
{
    int r ;

    r = 0 ;
    if (this->msg_ != 0 && m.msg_ != 0 && this->msglen_ == m.msglen_)
	r = std::memcmp (this->msg_, m.msg_, this->msglen_) == 0 ;
    return r ;
}

/******************************************************************************
 * Send and receive functions
 */

int msg::send (void)
{
    int r ;

    if (! msg_)
    {
	int i ;

	/*
	 * Format message, part 1 : compute message size
	 */

	msglen_ = 5 + toklen_ + paylen_ ;
	// XXX NO OPTION HANDLING FOR THE MOMENT

	if (id_ == 0)
	{
	    id_ = global_message_id++ ;
	    if (global_message_id > 0xffff)
		global_message_id = 1 ;
	}

	/*
	 * Format message, part 2 : build message
	 */

	msg_ = new byte [msglen_] ;

	i = 0 ;
	msg_ [i++] = FORMAT_BYTE0 (SOS_VERSION, type_, toklen_) ;
	msg_ [i++] = code_ ;
	msg_ [i++] = (id_ & 0xff00) >> 8 ;
	msg_ [i++] = id_ & 0xff ;
	if (toklen_ > 0)
	{
	    std::memcpy (msg_ + i, token_, toklen_) ;
	    i += toklen_ ;
	}
	// XXX NO OPTION HANDLING FOR THE MOMENT
	msg_ [i++] = 0xff ;			// start of payload
	std::memcpy (msg_ + i, payload_, paylen_) ;
    }

    std::cout << "TRANSMIT id=" << id_ << " ntrans_=" << ntrans_ << "\n" ;
    r = peer_->l2 ()->send (peer_->addr (), msg_, msglen_) ;
    if (r == -1)
    {
	std::cout << "ERREUR \n" ;
    }
    else
    {
	/*
	 * Timers for reliable messages
	 */

	if (type_ == MT_CON)
	{
	    if (ntrans_ == 0)
	    {
		long int nmilli ;
		int r ;

		/*
		 * initial timeout should be \in
		 *	[ACK_TIMEOUT ... ACK_TIMEOUT * ACK_RANDOM_FACTOR] (1)
		 *
		 * Let's name i = initial timeout, t = ACK_TIMEOUT and
		 * f = ACK_RANDOM_FACTOR
		 * (1)	<==> t <= i < t*f
		 *		<==> 1 <= i/t < f
		 *		<==> 0 <= (i/t) - 1 < f-1
		 *		<==> 0 <= ((i/t) - 1) * 1000 < (f-1)*1000
		 * So, we take a pseudo-random number r between 0 and (f-1)*1000
		 *		r = ((i/t) - 1) * 1000
		 * and compute i = t(r/1000 + 1) = t*(r + 1)/1000
		 */

		r = random_value (int ((ACK_RANDOM_FACTOR - 1.0) * 1000)) ;
		nmilli = ACK_TIMEOUT * (r + 1) ;
		nmilli = nmilli / 1000 ;
		timeout_ = std::chrono::milliseconds (nmilli) ;
	    }
	    else
	    {
		timeout_ *= 2 ;
	    }
	    next_timeout_ = std::chrono::system_clock::now () + timeout_ ;

	    ntrans_++ ;
	}
	else
	{
	    /*
	     * Non reliable messages : arbitrary set the retransmission
	     * counter in order to skip further retransmissions.
	     */

	    ntrans_ = MAX_RETRANSMIT ;
	}
    }
    return r ;
}

int msg::recv (l2net *l2, std::list <slave> slist)
{
    l2addr *a ;
    int len ;
    int r ;

    /*
     * Reset message object to a known state
     */

    RESET_POINTERS ;
    RESET_VALUES ;

    /*
     * Read message from L2 network
     */

    len = l2->mtu () ;
    msg_ = new byte [len] ;
    pktype_ = l2->recv (&a, msg_, &len) ; 	// create a l2addr *a
    std::cout << "RECV pkt=" << pktype_ << ", len=" << len << "\n" ;
    r = 0 ;				// recv failed
    if ((pktype_ == PK_ME || pktype_ == PK_BCAST)
    		&& COAP_VERSION (msg_) == SOS_VERSION)
    {
	/*
	 * Search originator slave
	 */

	std::list <slave>::iterator s ;
	int found ;

	found = 0 ;
	for (s = slist.begin () ; s != slist.end () ; s++)
	{
	    if (*a == *(s->addr ()))
	    {
		found = 1 ;
		break ;
	    }
	}

	if (found)
	{
	    int i ;
	    int opt_nb ;

	    /*
	     * By default, message is correct. Note that decoding may
	     * prove that message is in error
	     */

	    r = 1 ;			// recv succeeded

	    /*
	     * Decode message
	     */

	    peer_ = &*s ;
	    type_ = msgtype_t (COAP_TYPE (msg_)) ;
	    toklen_ = COAP_TOKLEN (msg_) ;
	    code_ = COAP_CODE (msg_) ;
	    id_ = COAP_ID (msg_) ;
	    i = 4 ;

	    if (toklen_ > 0)
	    {
		ALLOC_COPY (token_, msg_ + i, toklen_) ;
		i += toklen_ ;
	    }

	    /*
	     * Options XXXXXXXXXXXXXXXX
	     */

	    opt_nb = 0 ;
	    while (msg_ [i] != 0xff)
	    {
		int opt_delta, opt_len ;

		opt_delta = (msg_ [i] >> 4) & 0x0f ;
		opt_len   = (msg_ [i]     ) & 0x0f ;
		i++ ;
		switch (opt_delta)
		{
		    case 13 :
			opt_delta = msg_ [i] - 13 ;
			i += 1 ;
			break ;
		    case 14 :
			opt_delta = (msg_ [i] << 8) + msg_ [i+1] - 269 ;
			i += 2 ;
			break ;
		    case 15 :
			r = 0 ;			// recv failed
			break ;
		}
		opt_nb += opt_delta ;

		switch (opt_len)
		{
		    case 13 :
			opt_len = msg_ [i] - 13 ;
			i += 1 ;
			break ;
		    case 14 :
			opt_len = (msg_ [i] << 8) + msg_ [i+1] - 269 ;
			i += 2 ;
			break ;
		    case 15 :
			r = 0 ;			// recv failed
			break ;
		}

		/* get option value */
		// XXXX
		std::cout << "OPTION " << opt_nb << "\n" ;

		i += opt_len ;
	    }

	    ALLOC_COPY (payload_, msg_ + i, msglen_ - i) ;
	}
    }

    delete a ;				// remove address created by l2->recv ()

    return r ;
}

/******************************************************************************
 * Mutators
 */

void msg::peer (slave *p)
{
    peer_ = p ;
}

void msg::token (void *data, int len)
{
    if (token_)
	delete token_ ;

    ALLOC_COPY (token_, data, len) ;
    toklen_ = len ;
    RESET_BINARY ;
}

void msg::id (int id)
{
    id_ = id ;
    RESET_BINARY ;
}

void msg::type (msgtype_t type)
{
    type_ = type ;
    RESET_BINARY ;
}

void msg::code (int code)
{
    code_ = code ;
    RESET_BINARY ;
}

void msg::payload (void *data, int len)
{
    if (payload_)
	delete payload_ ;

    ALLOC_COPY (payload_, data, len) ;
    paylen_ = len ;
    RESET_BINARY ;
}

void msg::handler (reply_handler_t func)
{
    handler_ = func ;
}

/******************************************************************************
 * Accessors
 */

slave *msg::peer (void)
{
    return peer_ ;
}

pktype_t msg::pktype (void)
{
    return pktype_ ;
}

void *msg::token (int *toklen)
{
    *toklen = toklen_ ;
    return token_ ;
}

int msg::id (void)
{
    return id_ ;
}

msg::msgtype_t msg::type (void)
{
    return type_ ;
}

int msg::code (void)
{
    return code_ ;
}

void *msg::payload (int *paylen)
{
    *paylen = paylen_ ;
    return payload_ ;
}

bool msg::isanswer (void)
{
    /*********** NOT YET *****/
    return 1 ;
}

bool msg::isrequest (void)
{
    /*********** NOT YET *****/
    return 1 ;
}

bool msg::issosctl (void)
{
    /*********** NOT YET *****/
    return 1 ;
}

