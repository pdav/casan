#include "l2.h"
#include "req.h"

int request::global_message_id = 0 ;

request::request ()
{
    // timeout_ = 0 ;
    dl2 = 0 ;
    daddr = 0 ;
    // token_ = 0 ;
    id = global_message_id++ ;
    status = REQ_NONE ;
}

request::~request ()
{
}

/*****
request::timeout (int t)
{
    timeout_t = t ;
}
****/

void request::dest (l2net *l2, l2addr *a)
{
    dl2 = l2 ;
    daddr = a ;
}

/*****
request::token (int tok)
{
    token_ = &token ;
}
****/

void request::handler (req_handler_t func)
{
    handler_ = func ;
}
