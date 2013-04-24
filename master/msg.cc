#include "l2.h"
#include "msg.h"


#include <string.h>

int msg::global_message_id = 0 ;

msg::msg ()
{
    peer_ = 0 ;
    msg_ = 0 ; msglen_ = 0 ;
    payload_ = 0 ; paylen_ = 0 ;
    token_ = 0 ; token_ = 0 ;
    timeout_ = 0 ; ntrans_ = 0 ;
    id_ = global_message_id++ ;
}

msg::~msg ()
{
    if (payload_)
	delete payload_ ;
    if (token_)
	delete token_ ;
}

#define	RESET_MSG	if (msg_) delete msg_		// no ";"


/******************************************************************************
 * Send and receive functions
 */

void msg::reset (void)
{
}

void msg::send (void)
{
}

void msg::recv (l2net *l2)
{
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
    memcpy (token_, data, len) ;
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
    memcpy (payload_, data, len) ;
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
}

bool msg::isrequest (void)
{
    /*********** NOT YET *****/
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
