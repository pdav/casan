#include "resource.h"

#define	ALLOC_COPY(d,s)		do {				\
				    (d) = (char *) malloc (strlen (s) + 1) ; \
				    strcpy ((d), (s)) ;		\
				} while (false)			// no ";"

Resource::Resource (const char *name, const char *title, const char *rt)
{
    ALLOC_COPY (name_, name) ;
    ALLOC_COPY (title_, title) ;
    ALLOC_COPY (rt_, rt) ;
}

Resource::~Resource ()
{
    free (name_) ;
    free (title_) ;
    free (rt_) ;
}

bool Resource::check_name (const char *name) 
{
    return strcmp (name_, name) == 0 ;
}

void Resource::add_handler (coap_code_t type, uint8_t (*handler)(Msg &in, Msg &out)) 
{
    switch (type) {
	case COAP_CODE_GET :	thandler_ [0].handler = handler ; break ;
	case COAP_CODE_POST :	thandler_ [1].handler = handler ; break ;
	case COAP_CODE_PUT :	thandler_ [2].handler = handler ; break ;
	case COAP_CODE_DELETE :	thandler_ [3].handler = handler ; break ;
	default :
	    break ;
    }
}

handler_s Resource::get_handler (coap_code_t type) 
{
    switch (type) {
	case COAP_CODE_GET:	return thandler_ [0] ;
	case COAP_CODE_POST:	return thandler_ [1] ;
	case COAP_CODE_PUT:	return thandler_ [2] ;
	case COAP_CODE_DELETE:	return thandler_ [3] ;
	default :		return thandler_ [0] ;
    }
}

/*
 * Get a "well-known"-type text
 *	<temp>;title="Temperature";rt="celcius"
 * Returns length of string (including final \0), or -1 if it not fits
 * in the given buffer (of length maxlen)
 */

int Resource::get_well_known (char *buf, size_t maxlen)
{
    int len ;
    
    len = sizeof "<>;title=..;rt=.." ;		// including '\0'
    len += strlen (name_) + strlen (title_) + strlen (rt_) ;
    if (len > maxlen)
	len = -1 ;
    else
	sprintf (buf, "<%s>;title=\"%s\";rt=\"%s\"", name_, title_, rt_) ;

    return len ;
}

void Resource::print (void)
{
    Serial.print (F ("RES name=")) ;	Serial.print (name_) ;
    Serial.print (F (", title=")) ;	Serial.print (title_) ;
    Serial.print (F (", rt=")) ;	Serial.print (rt_) ;
    Serial.println () ;
}
