/**
 * @file resource.cpp
 * @brief Resource class implementation
 */

#include "resource.h"

#define	ALLOC_COPY(d,s)		do {				\
				    (d) = (char *) malloc (strlen (s) + 1) ; \
				    strcpy ((d), (s)) ;		\
				} while (false)			// no ";"

/** @brief Copy constructor
 */
Resource::Resource (const char *name, const char *title, const char *rt)
{
    ALLOC_COPY (name_, name) ;
    ALLOC_COPY (title_, title) ;
    ALLOC_COPY (rt_, rt) ;
    for (int i = 0 ; i < NTAB (handler_) ; i++)
	handler_ [i] = NULL ;
    observed_ = false ;
    obs_trig_ = NULL ;
    obs_reg_ = NULL ;
    obs_dereg_ = NULL ;
}

/** @brief Destructor
 */

Resource::~Resource ()
{
    free (name_) ;
    free (title_) ;
    free (rt_) ;
}

/** @brief Set resource handler
 *
 * @param op CoAP operation (see coap_code_t type)
 * @param h Function to handle request and provide a response
 */

void Resource::handler (coap_code_t op, handler_t h)
{
    handler_ [op] = h ;
}

/** @brief Get resource handler
 *
 * @param op CoAP operation (see coap_code_t type)
 * @return Handler for this operation
 */

Resource::handler_t Resource::handler (coap_code_t op)
{
    return handler_ [op] ;
}

/** @brief Set observe handlers
 *
 * @param reg handler for registering the observation (may be null)
 * @param dereg handler for deregistering the observation (may be null)
 * @param trigger handler for detecting a change (must not be null)
 */

void Resource::ohandler (obs_register_t reg, obs_deregister_t dereg, obs_trigger_t trig)
{
    observed_ = false ;
    obs_reg_ = reg ;
    obs_dereg_ = dereg ;
    obs_trig_ = trig ;
    obs_serial_ = 0 ;
}

/** @brief Register or deregister an observer
 *
 * @param onoff true (register) or false (deregister)
 * @param m incoming message (for registering) or NULL (for deregistering)
 */

void Resource::observed (bool onoff, Msg *m)
{
    if (obs_trig_ != NULL)
    {
	if (observed_ && obs_dereg_ != NULL)
	    (*obs_dereg_) () ;

	observed_ = onoff ;
	if (onoff)
	{
	    if (obs_reg_ != NULL)
		(*obs_reg_) (*m) ;
	    obs_serial_ = 2 ;			/* starting value */
	    obs_token_ = m->get_token () ;
	}
    }
}

/** @brief Detect observe events
 *
 * @return 1 if an observe message must be sent
 */

int Resource::check_trigger (void)
{
    return obs_trig_ == NULL ? 0 : (*obs_trig_) () ;
}


/** @brief Get the textual representation of the resource for the
 *	`/.well-known/casan` resource.
 *
 * The format of a "well-known"-type text is:
 *	<temp>;title="Temperature";rt="celcius"
 *
 * @param buf buffer where the textual representation must be stored
 * @param maxlen size of buffer
 * @return length of string (including final \0), or -1 if it not fits
 *	in the given buffer
 */

int Resource::well_known (char *buf, size_t maxlen)
{
    int len ;
    
    len = sizeof "<>;title=..;rt=.." ;		// including '\0'
    len += strlen (name_) + strlen (title_) + strlen (rt_) ;
    if (len > (int) maxlen)
	len = -1 ;
    else
	sprintf (buf, "<%s>;title=\"%s\";rt=\"%s\"", name_, title_, rt_) ;

    return len ;
}

/** @brief For debugging purposes
 */

void Resource::print (void)
{
    DBG1 (F ("RES name=")) ;	DBG1 (name_) ;
    DBG1 (F (", title=")) ;	DBG1 (title_) ;
    DBG1 (F (", rt=")) ;	DBG1 (rt_) ;
    DBGLN0 () ;
}
