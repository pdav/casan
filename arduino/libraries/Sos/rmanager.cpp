#include "rmanager.h"

Rmanager::Rmanager () 
{ 
    resources_ = NULL ;
}

Rmanager::~Rmanager () 
{ 
    reset () ;
}

void Rmanager::add_resource (Resource *r) 
{
    rmanager_list *newr ;
    
    newr = (rmanager_list *) malloc (sizeof (rmanager_list)) ;
    newr->next = resources_ ;
    newr->r = r ;
    resources_ = newr ;
}


// returns  0 if ok 
uint8_t Rmanager::request_resource (Message &in, Message &out) 
{
    uint8_t r = 1 ;

    in.reset_get_option () ;

    for (option *o = in.get_option () ; o != NULL ; o = in.get_option ())
    {
	if (o->optcode () == option::MO_Uri_Path)
	{
	    // if the request is to get all resources
	    if (o->optlen () >= (int) (sizeof SOS_RESOURCES_ALL -1) && 
			strncmp ((const char *) o->val (), 
				    SOS_RESOURCES_ALL, 
				    (sizeof SOS_RESOURCES_ALL -1)) == 0)
	    {
		out.set_type (COAP_TYPE_ACK) ;
		out.set_id (in.get_id ()) ;
		out.set_token (in.get_toklen (), in.get_token ()) ;
		out.set_code (COAP_RETURN_CODE (2,5)) ;
		get_all_resources (out) ;
		r = 0 ;
	    }
	    else
	    {
		rmanager_list *tmp ;
		Resource *res ;
		bool found ;

		tmp = get_resource_instance (o) ;
		if (tmp == NULL)
		    r = 1 ;

		out.set_type (COAP_TYPE_ACK) ;
		out.set_id (in.get_id ()) ;
		out.set_token (in.get_toklen (), in.get_token ()) ;
		
		res = tmp->r ;
		handler_s h = res->get_handler ((coap_code_t) in.get_code ()) ;
		r = (*h.handler) (in, out) ;

		found = false ;
		for (option *o2 = out.get_option () ; o2 != NULL ; o2 = out.get_option ())
		{
		    if (o2->optcode () == option::MO_Content_Format)
		    {
			found = true ;
			break ;
		    }
		}
		
		if (! found)
		{
		    option o3 ;

		    o3.optcode (option::MO_Content_Format) ;
		    o3.optval (option::cf_text_plain) ;
		    out.push_option (o3) ;
		}
	    }
	}
    }

    return r ;
}

void Rmanager::get_all_resources (Message &out) 
{
    char buf [150] ;
    int size = 0 ;

    for (int i = 0 ; i < (int) sizeof buf ; i++)
	buf [i] = '\0' ;

    // We have to send a message with a payload like : 
    // </temp> ;title="the temp" ;rt="Temp",</light> ;title="Luminosity" ;rt="light-lux"
    for (rmanager_list *tmp = resources_ ; tmp != NULL ; tmp = tmp->next) 
    {
	int len ;

	len = out.get_paylen () ;
	if (len)
	{
	    size = len + 1 ;		// 1 for ','
	    if (size < (int) sizeof buf)
	    {
		char *tmp = (char *) out.get_payload_copy () ;
		snprintf (buf, size + 1, "%s,", tmp ) ;
		free (tmp) ;
	    }
	}

	len = strlen (tmp->r->name_) + strlen (tmp->r->title_) + strlen (tmp->r->rt_) ; 
	size += len + 17 ; // for the '<> ;title="" ;rt=""'

	if (size < (int) sizeof buf)
	{
	    snprintf (buf, size +1, "%s<%s> ;title=\"%s\" ;rt=\"%s\"",
			    buf, tmp->r->name_, tmp->r->title_, tmp->r->rt_) ;
	    out.set_payload (size, (uint8_t *) buf) ;
	}
    }
}

void Rmanager::delete_resource (rmanager_list *r) 
{
    if (r == NULL)
    {
	return ;
    }
    else if (r == resources_)
    {
	resources_ = resources_->next ;
    }
    else
    {
	rmanager_list *tmp = resources_ ;
	while (tmp->next != r)
	    tmp = tmp->next ;
	tmp->next = r->next ;
    }
    delete r->r ;
    free (r) ;
}

void Rmanager::reset () 
{
    while (resources_ != NULL) {
	delete_resource (resources_) ;
    }
}

// We have to know how a resource will be requested
rmanager_list *Rmanager::get_resource_instance (option *o) 
{
    rmanager_list *tmp ;

    tmp = NULL ;
    for (tmp = resources_ ; tmp != NULL ; tmp = tmp->next)
	if (tmp->r->check_name ((char *) o->val (), o->optlen ()))
	    break ;

    return tmp ;
}

void Rmanager::print (void)
{
    char buf [150] ;

    for (rmanager_list *tmp = resources_ ; tmp != NULL ; tmp = tmp->next)
    {
	Serial.print (F ("resource name : ")) ;
	memcpy (buf, tmp->r->name_, strlen (tmp->r->name_)+1) ;
	Serial.println (buf) ;

	Serial.print (F ("resource title : ")) ;
	memcpy (buf, tmp->r->title_, strlen (tmp->r->title_)+1) ;
	Serial.println (buf) ;

	Serial.print (F ("resource rt : ")) ;
	memcpy (buf, tmp->r->rt_, strlen (tmp->r->rt_)+1) ;
	Serial.println (buf) ;
    }
}
