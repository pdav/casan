#include "rmanager.h"

Rmanager::Rmanager () 
{ 
    resources_ = NULL ;
}

Rmanager::~Rmanager () 
{ 
    reset () ;
}

void Rmanager::add_resource (Resource *res) 
{
    rmanager_list *newr ;
    
    newr = new rmanager_list ;
    newr->next = resources_ ;
    newr->res = res ;
    resources_ = newr ;
}

void Rmanager::request_resource (Msg &in, Msg &out) 
{
    option *o ;
    bool rfound = false ;		// resource found

    in.reset_next_option () ;
    for (o = in.next_option () ; o != NULL ; o = in.next_option ())
    {
	if (o->optcode () == option::MO_Uri_Path)
	{
	    // request for all resources
	    if (o->optlen () == (int) (sizeof SOS_RESOURCES_ALL - 1)
		&& memcmp (o->val (), SOS_RESOURCES_ALL, 
				    sizeof SOS_RESOURCES_ALL - 1) == 0)
	    {
		rfound = true ;
		out.set_type (COAP_TYPE_ACK) ;
		out.set_id (in.get_id ()) ;
		out.set_token (in.get_toklen (), in.get_token ()) ;
		out.set_code (COAP_RETURN_CODE (2, 5)) ;
		get_resource_list (out) ;
	    }
	    else
	    {
		Resource *res ;
		option *ocf ;
		uint8_t r ;

		print () ;
		res = resource_by_name ((char *) o->val (), o->optlen ()) ;
		if (res != NULL)
		{
		    rfound = true ;
		    out.set_type (COAP_TYPE_ACK) ;
		    out.set_id (in.get_id ()) ;
		    out.set_token (in.get_toklen (), in.get_token ()) ;

		    // call handler
		    handler_s h = res->get_handler ((coap_code_t) in.get_code ()) ;
		    r = (*h.handler) (in, out) ;
		    out.set_code (r) ;

		    // add Content Format option if not created by handler
		    out.reset_next_option () ;
		    for (ocf = out.next_option () ; ocf != NULL ; ocf = out.next_option ())
			if (ocf->optcode () == option::MO_Content_Format)
			    break ;
		    
		    if (ocf == NULL)
		    {
			ocf = new option (option::MO_Content_Format, option::cf_text_plain) ;
			out.push_option (*ocf) ;
			delete ocf ;
		    }
		}
	    }
	    break ;
	}
    }

    if (! rfound)
    {
	out.set_type (COAP_TYPE_ACK) ;
	out.set_id (in.get_id ()) ;
	out.set_token (in.get_toklen (), in.get_token ()) ;
	out.set_code (COAP_RETURN_CODE (4, 4)) ;
    }
}

/*
 * Prepare the payload for an assoc answer message (answer to the
 *	CON POST /.well-known/sos ? assoc=<sttl>
 * message).
 *
 * The answer will have a payload similar to:
 *	</temp> ;
 *		title="the temp";
 *		rt="Temp",</light>;
 *		title="Luminosity";
 *		rt="light-lux"
 * (with the newlines removed)
 */

void Rmanager::get_resource_list (Msg &out) 
{
    char buf [150] ;				// XXX
    int size ;
    rmanager_list *r ;

    size = 0 ;
    for (r = resources_ ; r != NULL ; r = r->next) 
    {
	int len = sizeof "<>;title=..;rt=.." - 1 ;	// without '\0'

	len += strlen (r->res->name_) ;
	len += strlen (r->res->title_) ;
	len += strlen (r->res->rt_) ;

	if (size > 0)
	{
	    if (size + 1 < (int) sizeof buf)
		buf [size++] = ',' ;
	    else
		break ;
	}

	if (size + len < (int) sizeof buf)
	{
	    // adds an unused '\0' at the end: keep enough space for this byte!
	    sprintf (buf + size, "<%s>;title=\"%s\";rt=\"%s\"",
				r->res->name_, r->res->title_, r->res->rt_) ;
	    size += len ;
	}
	else break ;
    }
    out.set_payload ((uint8_t *) buf, size) ;
}

void Rmanager::delete_resource (rmanager_list *rl) 
{
    rmanager_list *cur, *prev ;

    prev = NULL ;
    cur = resources_ ;
    while (cur != NULL)
    {
	if (cur == rl)
	{
	    if (prev == NULL)
		resources_ = cur->next ;
	    else
		prev->next = cur->next ;
	    delete cur->res ;
	    delete cur ;
	    cur = NULL ;			// leave loop
	}
	else cur = cur->next ;
    }
}

void Rmanager::reset (void) 
{
    while (resources_ != NULL)
	delete_resource (resources_) ;
}

// Find a particular resource by its name
Resource *Rmanager::resource_by_name (const char *name, int len)
{
    rmanager_list *rl ;

    for (rl = resources_ ; rl != NULL ; rl = rl->next)
	if (rl->res->check_name (name, len))
	    break ;
    return rl != NULL ? rl->res : NULL ;
}

void Rmanager::print (void)
{
    rmanager_list *rl ;

    for (rl = resources_ ; rl != NULL ; rl = rl->next)
	rl->res->print () ;
}
