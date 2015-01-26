#include "retrans.h"

Retrans::Retrans () 
{
    retransq_ = NULL ;
}

Retrans::~Retrans () 
{
    reset () ;
}

void Retrans::reset (void) 
{
    while (retransq_ != NULL)
	del (NULL, retransq_) ;
}

void Retrans::master (l2addr **master)
{
    master_addr_ = master ;
}

// insert a new message in the retransmission list
void Retrans::add (Msg *msg) 
{
    retransq *n ;

    sync_time (curtime) ;		// synchronize curtime

    n = (retransq *) malloc (sizeof (retransq)) ;
    n->msg = msg ;
    n->timelast = curtime ;
    n->timenext = curtime + ALEA (ACK_TIMEOUT * ACK_RANDOM_FACTOR) ;
    n->ntrans = 0 ;
    n->next = retransq_ ;
    retransq_ = n ;
}

void Retrans::del (Msg *msg) 
{
    del (get (msg)) ;
}

// TODO
void Retrans::loop (l2net &l2, time_t &curtime)
{
    // DBGLN1 (F ("retransmit loop")) ;

    if (*master_addr_ == NULL)
    {
	// master has been reset by the CASAN engine (waiting-unknown state)
	reset () ;
	return ;
    }
    else
    {
	retransq *cur, *prev ;

	prev = NULL ;
	cur = retransq_ ;
	while (cur != NULL)
	{
	    retransq *next ;

	    next = cur->next ;
	    if (cur->ntrans >= MAX_RETRANSMIT)
	    {
		// remove the message from the queue
		del (prev, cur) ;
		// prev is not modified
	    }
	    else
	    {
		if (cur->timenext < curtime)
		{
		    cur->msg->send (**master_addr_) ;
		    cur->ntrans++ ;
		    cur->timenext = cur->timenext + (2* (cur->timenext - cur->timelast));
		    sync_time (cur->timelast) ;
		}
		prev = cur ;
	    }
	    cur = next ;
	}
    }
}

void Retrans::check_msg_received (Msg &in) 
{
    switch (in.get_type ())
    {
	case COAP_TYPE_ACK :
	    del (&in) ;
	    break ;
	default :
	    break ;
    }
}

void Retrans::check_msg_sent (Msg &in) 
{
    switch (in.get_type ())
    {
	case COAP_TYPE_CON :
	    add (&in) ;
	    break ;
	default :
	    break ;
    }
}

// for internal use only
void Retrans::del (retransq *r) 
{
    retransq *cur, *prev ;

    prev = NULL ;
    for (cur = retransq_ ; cur != NULL ; cur = cur->next)
    {
	if (cur == r)
	    break ;
	prev = cur ;
    }

    if (cur != NULL)
	del (prev, cur) ;
}

// for internal use only
void Retrans::del (retransq *prev, retransq *cur) 
{
    if (prev == NULL)
	retransq_ = cur->next ;
    else
	prev->next = cur->next ;
    if (cur->msg != NULL)
	delete cur->msg ;
    free (cur) ;
}

// get a message to retransmit, given its message id
Retrans::retransq *Retrans::get (Msg *msg) 
{
    retransq *cur ;

    for (cur = retransq_ ; cur != NULL ; cur = cur->next)
    {
	// TODO : maybe check the token too
	if (cur->msg->get_id () == msg->get_id ())
	    break ;
    }
    return cur ;
}
