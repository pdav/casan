#include "retransmit.h"

Retransmit::Retransmit (Coap *c, l2addr **master) 
{
    coap_ = c ;
    master_addr_ = master ;
    retransq_ = NULL ;
}

Retransmit::~Retransmit () 
{
    reset () ;
}

void Retransmit::reset (void) 
{
    while (retransq_ != NULL)
	del (NULL, retransq_) ;
}

// insert a new message in the retransmission list
void Retransmit::add (Message *m) 
{
    retransq *n ;

    current_time.cur () ;		// synchronize current_time

    n = (retransq *) malloc (sizeof (retransq)) ;
    n->m = m ;
    n->timelast = current_time ;
    n->timenext = current_time ;
    n->timenext.add (ALEA (ACK_TIMEOUT * ACK_RANDOM_FACTOR)) ;
    n->ntrans = 0 ;
    n->next = retransq_ ;
    retransq_ = n ;
}

void Retransmit::del (Message *m) 
{
    del (get_retransmit (m)) ;
}

// TODO
void Retransmit::loop_retransmit (void) 
{
    retransq *cur, *prev ;

    Serial.println (F ("retransmit loop")) ;

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
	    if (cur->timenext < current_time)
	    {
		coap_->send (**master_addr_, *(cur->m)) ;
		cur->ntrans++ ;
		cur->timenext.add (2*time::diff (cur->timenext, cur->timelast));
		cur->timelast.cur () ;
	    }
	    prev = cur ;
	}
	cur = next ;
    }
}

void Retransmit::check_msg_received (Message &in) 
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

void Retransmit::check_msg_sent (Message &in) 
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
void Retransmit::del (retransq *r) 
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
void Retransmit::del (retransq *prev, retransq *cur) 
{
    if (prev == NULL)
	retransq_ = cur->next ;
    else
	prev->next = cur->next ;
    if (cur->m != NULL)
	delete cur->m ;
    free (cur) ;
}

// get a message to retransmit, given its message id
Retransmit::retransq *Retransmit::get_retransmit (Message *m) 
{
    retransq *cur ;

    for (cur = retransq_ ; cur != NULL ; cur = cur->next)
    {
	// TODO : maybe check the token too
	if (cur->m->get_id () == m->get_id ())
	    break ;
    }
    return cur ;
}

