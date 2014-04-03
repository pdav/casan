#include "resource.h"

Resource::Resource (const char *name, const char *title, const char *rt)
{
    name_ = (char *) malloc (strlen (name) + 1) ;
    strcpy (name_, name) ;

    title_ = (char *) malloc (strlen (title) + 1) ;
    strcpy (title_, title) ;

    rt_ = (char *) malloc (strlen (rt) + 1) ;
    strcpy (rt_, rt) ;
}

bool Resource::check_name (const char *name, int len) 
{
    return strncmp (name_, name, len) == 0 ;
}

void Resource::add_handler (coap_code_t type, uint8_t (*handler)(Message &in, Message &out)) 
{
    switch (type) {
	case COAP_CODE_GET :
	    thandler_ [0].handler = handler ;
	    break ;
	case COAP_CODE_POST :
	    thandler_ [1].handler = handler ;
	    break ;
	case COAP_CODE_PUT :
	    thandler_ [2].handler = handler ;
	    break ;
	case COAP_CODE_DELETE :
	    thandler_ [3].handler = handler ;
	    break ;
	default :
	    break ;
    }
}

handler_s Resource::get_handler (coap_code_t type) 
{
    switch (type) {
	case COAP_CODE_GET:
	    return thandler_ [0] ;
	case COAP_CODE_POST:
	    return thandler_ [1] ;
	case COAP_CODE_PUT:
	    return thandler_ [2] ;
	case COAP_CODE_DELETE:
	    return thandler_ [3] ;
	default :
	    return thandler_ [0] ;
    }
}
