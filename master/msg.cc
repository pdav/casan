#include "l2.h"
#include "msg.h"

int msg::global_message_id = 0 ;

msg::msg ()
{
    // timeout_ = 0 ;
    dl2 = 0 ;
    daddr = 0 ;
    // token_ = 0 ;
    id = global_message_id++ ;
    status = REQ_NONE ;
}

msg::~msg ()
{
}

void msg::dest (l2net *l2, l2addr *a)
{
    dl2 = l2 ;
    daddr = a ;
}

/*****
msg::token (int tok)
{
    token_ = &token ;
}
****/

void msg::handler (req_handler_t func)
{
    handler_ = func ;
}
