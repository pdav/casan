#include <iostream>
#include <cstring>

#include "defs.h"
#include "sos.h"
#include "l2.h"
#include "msg.h"
#include "slave.h"
#include "utils.h"

int msg::global_message_id = 0 ;

msg::msg ()
{
    peer_ = 0 ;
    msg_ = 0     ; msglen_ = 0 ;
    payload_ = 0 ; paylen_ = 0 ;
    token_ = 0   ; toklen_ = 0 ;
    timeout_ = std::chrono::milliseconds (0) ;
    ntrans_ = 0 ;

    id_ = global_message_id++ ;
}

msg::~msg ()
{
    if (msg_)
	delete msg_ ;
    if (payload_)
	delete payload_ ;
    if (token_)
	delete token_ ;
}

#define	RESET_MSG	if (msg_) delete msg_		// no ";"


/******************************************************************************
 * Send and receive functions
 */

#define	FORMAT_BYTE0(ver,type,toklen) \
			((((unsigned int) (ver) & 0x3) << 6) | \
			 (((unsigned int) (type) & 0x3) << 4) | \
			 (((unsigned int) (toklen) & 0x7)) \
			 )

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

	/*
	 * Format message, part 2 : build message
	 */

	msg_ = new unsigned char [msglen_] ;

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

int msg::recv (l2net *l2)
{
    return sizeof l2 ;		// keep -Wall silent
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

    token_ = new unsigned char [len] ;
    std::memcpy (token_, data, len) ;
    toklen_ = len ;
    RESET_MSG ;
}

void msg::type (msgtype_t type)
{
    type_ = type ;
    RESET_MSG ;
}

void msg::code (int code)
{
    code_ = code ;
    RESET_MSG ;
}

void msg::payload (void *data, int len)
{
    if (payload_)
	delete payload_ ;

    payload_ = new unsigned char [len] ;
    std::memcpy (payload_, data, len) ;
    paylen_ = len ;
    RESET_MSG ;
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

int msg::code (void)
{
    return code_ ;
}

void *msg::payload (int *paylen)
{
    *paylen = paylen_ ;
    return payload_ ;
}
