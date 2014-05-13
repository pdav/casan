/**
 * @file sos.cpp
 * @brief Sos class implementation
 */

#include "sos.h"

#define	SOS_NAMESPACE1		".well-known"
#define	SOS_NAMESPACE2		"sos"
#define	SOS_HELLO		"hello=%ld"
#define	SOS_DISCOVER_SLAVEID	"slave=%ld"
#define	SOS_DISCOVER_MTU	"mtu=%d"
#define	SOS_ASSOC		"assoc=%ld"

#define	SOS_BUF_LEN		50	// > sizeof hello=.../slave=..../etc


#define SOS_RESOURCES_ALL	"resources"

static struct
{
    const char *path ;
    int len ;
} sos_namespace [] =
{
    {  SOS_NAMESPACE1, sizeof SOS_NAMESPACE1 - 1 },
    {  SOS_NAMESPACE2, sizeof SOS_NAMESPACE2 - 1 },
} ;

/******************************************************************************
Constructor and simili-destructor
******************************************************************************/

/**
 * @brief Constructor
 *
 * The constructor method needs an already initialized L2 network
 * object, and a slave-id.
 *
 * @param l2 pointer to an already initialized network object (l2net-*)
 * @param slaveid unique slave-id
 */

Sos::Sos (l2net *l2, long int slaveid)
{
    memset (this, 0, sizeof *this) ;

    l2_ = l2 ;
    slaveid_ = slaveid ;

    curtime = 0 ;			// global variable
    sync_time (curtime) ;

    reset_master () ;			// master_ is reset to broadcast

    hlid_ = -1 ;
    curid_ = 1 ;

    retrans_.master (&master_) ;
    status_ = SL_COLDSTART ;
}

/*
 * @brief Reset SOS engine
 *
 * This method is used to reset the SOS engine.
 *
 * @bug as this method is not used, implementation is very rudimentary.
 *	One should restart all the engine, delete all exchange history, etc.
 */

void Sos::reset (void)
{
    status_ = SL_COLDSTART ;
    curid_ = 1 ;

    // remove resources from the list
    while (reslist_ != NULL)
    {
	reslist *r ;

	r = reslist_->next ;
	delete reslist_ ;
	reslist_ = r ;
    }

    retrans_.reset () ;
    reset_master () ;
}

/******************************************************************************
Master handling
******************************************************************************/

/**
 * Reset master coordinates to an unknown master:
 * * address is null
 * * hello-id is -1 (i.e. unknown hello-id)
 */

void Sos::reset_master (void)
{
    if (master_ != NULL)
	delete master_ ;
    master_ = NULL ;
    hlid_ = -1 ;

    Serial.println (F ("Master reset to broadcast address")) ;
}

/**
 * Does master address match the given address (which cannot be a
 * NULL pointer)?
 */

bool Sos::same_master (l2addr *a)
{
    return master_ != NULL && *a == *master_ ;
}

/**
 * Change master to a known master.
 * - address is taken from the current incoming message
 * - hello-id is given, may be -1 if value is currently not known
 */

void Sos::change_master (long int hlid)
{
    l2addr *newmaster ;

    newmaster = l2_->get_src () ;	// get a new address
    if (master_ != NULL)
    {
	if (*newmaster == *master_)
	{
	    if (hlid != -1)
		hlid_ = hlid ;
	    delete newmaster ;
	}
	else
	{
	    delete master_ ;
	    master_ = newmaster ;
	    hlid_ = hlid ;
	}
    }
    else
    {
	master_ = newmaster ;
	hlid_ = hlid ;
    }

    Serial.print (F ("Master set to ")) ;
    master_->print () ;
    Serial.print (F (", helloid=")) ;
    Serial.print (hlid_) ;
    Serial.println () ;
}

/******************************************************************************
Resource handling
******************************************************************************/

/**
 * @brief Register a resource to the SOS engine
 *
 * This method is used to register a resource with the SOS engine.
 * This resource will then be advertised with the `/.well-known/sos`
 * resource during the next association or with a specific request
 * from the master. Thus, registering a resource after an association
 * (when the slave returns an association answer containing the
 * `/.well-known/sos`) will not provoke a new association. One must
 * wait the next association renewal for the resource to be published
 * and thus known by the master.
 *
 * @param res Address of the resource to register
 * @return true if resource have been added successfully
 */

bool Sos::register_resource (Resource *res)
{
    reslist *newr, *prev, *cur ;
    bool ok ;

    /*
     * Register resource in last position of the list to respect
     * order provided by the application
     */

    newr = new reslist ;
    newr->res = res ;

    prev = NULL ;
    for (cur = reslist_ ; cur != NULL ; cur = cur->next)
	prev = cur ;
    if (prev != NULL)
	prev->next = newr ;
    else
	reslist_ = newr ;
    newr->next = NULL ;

    /*
     * Simulate creation of a new message in order to see if it fits
     */

    if (l2_ != NULL)
    {
	Msg fake (l2_) ;
	ok = get_well_known (fake) ;	// error message if does not fit
    }
    else ok = true ;

    return ok ;
}

/**
 * @brief Process an incoming message requesting for a resource
 *
 * This methods:
 * * analyze uri_path option to find the resource
 * * either give answer if this is the /resources URI
 * * or call the handler for user-defined resources
 * * or return 4.04 code
 * * pack the answer in the outgoing message
 *
 * This method is made public for testing purpose.
 *
 * @bug Only one level of path is allowed (i.e. /a, and not /a/b nor /a/b/c)
 *
 * @param in Incoming message
 * @param out Message which will be sent in return
 */

void Sos::request_resource (Msg &in, Msg &out) 
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
		&& memcmp (o->optval ((int *) 0), SOS_RESOURCES_ALL, 
				    sizeof SOS_RESOURCES_ALL - 1) == 0)
	    {
		rfound = true ;
		out.set_type (COAP_TYPE_ACK) ;
		out.set_id (in.get_id ()) ;
		out.set_token (in.get_toklen (), in.get_token ()) ;
		out.set_code (COAP_CODE_OK) ;
		(void) get_well_known (out) ;
	    }
	    else
	    {
		Resource *res ;

		// we benefit from the added '\0' at the end of an option
		res = get_resource ((char *) o->optval ((int *) 0)) ;
		if (res != NULL)
		{
		    rfound = true ;
		    out.set_type (COAP_TYPE_ACK) ;
		    out.set_id (in.get_id ()) ;
		    out.set_token (in.get_toklen (), in.get_token ()) ;

		    // call handler
		    Resource::handler_t h ;
		    
		    h = res->handler ((coap_code_t) in.get_code ()) ;
		    if (h == NULL)
		    {
			out.set_code (COAP_CODE_BAD_REQUEST) ;
		    }
		    else
		    {
			// add Content Format option
			out.content_format (false, option::cf_text_plain) ;
			out.set_code ((*h) (in, out)) ;
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
	out.set_code (COAP_CODE_NOT_FOUND) ;
    }
}

/**
 * Find a particular resource by its name
 */

Resource *Sos::get_resource (const char *name)
{
    reslist *rl ;

    for (rl = reslist_ ; rl != NULL ; rl = rl->next)
	if (strcmp (name, rl->res->name ()) == 0)
	    break ;
    return rl != NULL ? rl->res : NULL ;
}

/**
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
 *
 * @return true if message is succesfully built (enough space)
 */

bool Sos::get_well_known (Msg &out) 
{
    char *buf ;
    size_t size ;
    reslist *rl ;
    size_t avail ;
    bool reset ;

    reset = false ;
    out.content_format (reset, option::cf_text_plain) ;

    avail = out.avail_space () ;
    buf = (char *) malloc (avail) ;

    size = 0 ;
    for (rl = reslist_ ; rl != NULL ; rl = rl->next) 
    {
	int len ;

	if (size > 0)			// separator "," between resources
	{
	    if (size + 2 < avail)
	    {
		buf [size++] = ',' ;
		buf [size] = '\0' ;
	    }
	    else break ;		// too large
	}

	len = rl->res->well_known (buf + size, avail - size) ;
	if (len == -1)
	    break ;

	size += len - 1 ;		// exclude '\0'
    }

    out.set_payload ((uint8_t *) buf, size) ;
    free (buf) ;


    /*
     * Did all resources fitted in the message, or do we left the loop
     * before its term?
     */

    if (rl != NULL)
    {
	Serial.print (F (B_RED "Resource '")) ;
	Serial.print (rl->res->name ()) ;
	Serial.print (F ("' do not fit in buffer of ")) ;
	Serial.print (avail) ;
	Serial.println (F (" bytes" C_RESET)) ;
    }

    return rl == NULL ;			// true if all res are in the message
}

/******************************************************************************
Main SOS loop
******************************************************************************/

/**
 * @brief Main SOS loop
 *
 * This method must be called regularly (typically in the loop function
 * of the Arduino framework) in order to process SOS events.
 */

void Sos::loop ()
{
    Msg in (l2_) ;
    Msg out (l2_) ;
    l2net::l2_recv_t ret ;
    uint8_t oldstatus ;
    long int hlid ;
    l2addr *srcaddr ;

    oldstatus = status_ ;		// keep old value for debug display
    sync_time (curtime) ;		// get current time
    retrans_.loop (*l2_, curtime) ;	// check needed retransmissions

    srcaddr = NULL ;

    ret = in.recv () ;			// get received message
    if (ret == l2net::RECV_OK)
	srcaddr = l2_->get_src () ;	// get a new address

    switch (status_)
    {
	case SL_COLDSTART :
	    send_discover (out) ;
	    twait_.init (curtime) ;
	    status_ = SL_WAITING_UNKNOWN ;
	    break ;

	case SL_WAITING_UNKNOWN :
	    if (ret == l2net::RECV_OK)
	    {
		retrans_.check_msg_received (in) ;

		if (is_ctl_msg (in))
		{
		    if (is_hello (in, hlid))
		    {
			Serial.println (F ("Received a CTL HELLO msg")) ;
			change_master (hlid) ;
			twait_.init (curtime) ;
			status_ = SL_WAITING_KNOWN ;
		    }
		    else if (is_assoc (in, sttl_))
		    {
			Serial.println (F ("Received a CTL ASSOC msg")) ;
			hlid = -1 ;	// "unknown" hlid
			change_master (hlid) ;
			send_assoc_answer (in, out) ;
			trenew_.init (curtime, sttl_) ;
			status_ = SL_RUNNING ;
		    }
		    else Serial.println (F ("\033[31mUnkwnon CTL\033[00m")) ;
		}
	    }

	    if (status_ == SL_WAITING_UNKNOWN && twait_.next (curtime))
		send_discover (out) ;

	    break ;

	case SL_WAITING_KNOWN :
	    if (ret == l2net::RECV_OK)
	    {
		retrans_.check_msg_received (in) ;

		if (is_ctl_msg (in))
		{
		    if (is_hello (in, hlid))
		    {
			Serial.println (F ("Received a CTL HELLO msg")) ;
			change_master (hlid) ;
		    }
		    else if (is_assoc (in, sttl_))
		    {
			Serial.println (F ("Received a CTL ASSOC msg")) ;
			hlid = -1 ;		// unknown hlid
			change_master (hlid) ;
			send_assoc_answer (in, out) ;
			trenew_.init (curtime, sttl_) ;
			status_ = SL_RUNNING ;
		    }
		    else Serial.println (F ("\033[31Unknown CTL\033[00m")) ;
		}
	    }

	    if (status_ == SL_WAITING_KNOWN)
	    {
		if (twait_.expired (curtime))
		{
		    reset_master () ;		// master_ is no longer known
		    send_discover (out) ;
		    twait_.init (curtime) ;	// reset timer
		    status_ = SL_WAITING_UNKNOWN ;
		}
		else if (twait_.next (curtime))
		{
		    send_discover (out) ;
		}
	    }

	    break ;

	case SL_RUNNING :
	case SL_RENEW :
	    if (ret == l2net::RECV_OK)
	    {
		retrans_.check_msg_received (in) ;

		if (is_ctl_msg (in))
		{
		    if (is_hello (in, hlid))
		    {
			Serial.println (F ("Received a CTL HELLO msg")) ;
			if (! same_master (srcaddr) || hlid != hlid_)
			{
			    change_master (hlid) ;
			    twait_.init (curtime) ;
			    status_ = SL_WAITING_KNOWN ;
			}
		    }
		    else if (is_assoc (in, sttl_))
		    {
			Serial.println (F ("Received a CTL ASSOC msg")) ;
			if (same_master (srcaddr))
			{
			    send_assoc_answer (in, out) ;
			    trenew_.init (curtime, sttl_) ;
			    status_ = SL_RUNNING ;
			}
		    }
		    else Serial.println (F ("\033[31Unknown CTL\033[00m")) ;
		}
		else		// request for a normal resource
		{
		    // deduplicate () ;
		    request_resource (in, out) ;
		    out.send (*master_) ;
		}
	    }
	    else if (ret == l2net::RECV_TRUNCATED)
	    {
		Serial.println (F ("\033[36mRequest too large\033[00m")) ;
		out.set_type (COAP_TYPE_ACK) ;
		out.set_id (in.get_id ()) ;
		out.set_token (in.get_toklen (), in.get_token ()) ;
		option o (option::MO_Size1, l2_->mtu ()) ;
		out.push_option (o) ;
		out.set_code (COAP_CODE_TOO_LARGE) ;
		out.send (*master_) ;
	    }

	    if (status_ == SL_RUNNING && trenew_.renew (curtime))
	    {
		send_discover (out) ;
		status_ = SL_RENEW ;
	    }

	    if (status_ == SL_RENEW && trenew_.next (curtime))
	    {
		send_discover (out) ;
	    }

	    if (status_ == SL_RENEW && trenew_.expired (curtime))
	    {
		reset_master () ;	// master_ is no longer known
		send_discover (out) ;
		twait_.init (curtime) ;	// reset timer
		status_ = SL_WAITING_UNKNOWN ;
	    }

	    break ;

	default :
	    Serial.println (F ("Error : sos status not known")) ;
	    Serial.println (status_) ;
	    break ;
    }

    if (oldstatus != status_)
    {
	Serial.print (F (C_GREEN "Status: ")) ;
	print_status (oldstatus) ;
	Serial.print (F (" -> ")) ;
	print_status (status_) ;
	Serial.println (F (C_RESET)) ;
    }

    if (srcaddr != NULL)
	delete srcaddr ;
}

/******************************************************************************
Recognize control messages
******************************************************************************/

/**
 * Is the incoming message an SOS control message?
 * Just verify if Uri_Path options match the sos_namespace [] array
 * in the right order
 */

bool Sos::is_ctl_msg (Msg &m)
{
    int i = 0 ;

    m.reset_next_option () ;
    for (option *o = m.next_option () ; o != NULL ; o = m.next_option ())
    {
	if (o->optcode () == option::MO_Uri_Path)
	{
	    if (i >= NTAB (sos_namespace))
		return false ;
	    if (sos_namespace [i].len != o->optlen ())
		return false ;
	    if (memcmp (sos_namespace [i].path, o->optval ((int *) 0), o->optlen ()))
		return false ;
	    i++ ;
	}
    }
    m.reset_next_option () ;
    if (i != NTAB (sos_namespace))
	return false ;

    return true ;
}

/**
 * Check if the control message is a Hello message from the master
 * and returns the contained hello-id
 */

bool Sos::is_hello (Msg &m, long int &hlid)
{
    bool found = false ;

    // a hello msg is NON POST
    if (m.get_type () == COAP_TYPE_NON && m.get_code () == COAP_CODE_POST)
    {
	m.reset_next_option () ;
	for (option *o = m.next_option () ; o != NULL ; o = m.next_option ())
	{
	    if (o->optcode () == option::MO_Uri_Query)
	    {
		// we benefit from the added nul byte at the end of val
		if (sscanf ((const char *) o->optval ((int *) 0), SOS_HELLO, &hlid) == 1)
		    found = true ;
	    }
	}
    }

    return found ;
}

/**
 * Check if the control message is an Assoc message from the master
 * and returns the contained slave-ttl
 */

bool Sos::is_assoc (Msg &m, time_t &sttl)
{
    bool found = false ;

    if (m.get_type () == COAP_TYPE_CON && m.get_code () == COAP_CODE_POST)
    {
	m.reset_next_option () ;
	for (option *o = m.next_option () ; o != NULL ; o = m.next_option ())
	{
	    if (o->optcode () == option::MO_Uri_Query)
	    {
		long int n ;		// sscanf "%ld" waits for a long int

		// we benefit from the added nul byte at the end of val
		if (sscanf ((const char *) o->optval ((int *) 0), SOS_ASSOC, &n) == 1)
		{
		    Serial.print (F ("\033[31m TTL recv \033[00m ")) ;
		    Serial.print (n) ;
		    Serial.println () ;
		    sttl = ((time_t) n) * 1000 ;
		    found = true ;
		    // continue, just in case there are other query strings
		}
		else
		{
		    found = false ;
		    break ;
		}
	    }
	}
    }

    return found ;
}

/******************************************************************************
Send control messages
******************************************************************************/

/**
 * Initialize an "empty" control message
 * Just add the Uri_Path options from the sos_namespace [] array
 */

void Sos::mk_ctl_msg (Msg &out)
{
    int i ;

    for (i = 0 ; i < NTAB (sos_namespace) ; i++)
    {
	option path (option::MO_Uri_Path, (void *) sos_namespace [i].path,
					sos_namespace [i].len) ;
	out.push_option (path) ;
    }
}

/**
 * Send a discover message
 */

void Sos::send_discover (Msg &out)
{
    char tmpstr [SOS_BUF_LEN] ;
    l2addr *dest ;

    Serial.println (F ("Sending Discover")) ;

    out.reset () ;
    out.set_id (curid_++) ;
    out.set_type (COAP_TYPE_NON) ;
    out.set_code (COAP_CODE_POST) ;
    mk_ctl_msg (out) ;

    snprintf (tmpstr, sizeof tmpstr, SOS_DISCOVER_SLAVEID, slaveid_) ;
    option o1 (option::MO_Uri_Query, tmpstr, strlen (tmpstr)) ;
    out.push_option (o1) ;

    snprintf (tmpstr, sizeof tmpstr, SOS_DISCOVER_MTU, l2_->mtu ()) ;
    option o2 (option::MO_Uri_Query, tmpstr, strlen (tmpstr)) ;
    out.push_option (o2) ;

    dest = (master_ != NULL) ? master_ : l2_->bcastaddr () ;

    out.send (*dest) ;
}

/**
 * Send the answer to an association message
 * (the association task itself is handled in the SOS main loop)
 */

void Sos::send_assoc_answer (Msg &in, Msg &out)
{
    l2addr *dest ;

    dest = l2_->get_src () ;

    // send back an acknowledgement message
    out.set_type (COAP_TYPE_ACK) ;
    out.set_code (COAP_CODE_OK) ;
    out.set_id (in.get_id ()) ;

    // will get the resources and set them in the payload in the right format
    (void) get_well_known (out) ;

    // send the packet
    if (! out.send (*dest))
	Serial.println (F (RED ("Cannot send the assoc answer message"))) ;

    delete dest ;
}

/******************************************************************************
Debug methods
******************************************************************************/

/**
 * @brief Print the list of resources, used for debug purpose
 */

void Sos::print_resources (void)
{
    reslist *rl ;

    for (rl = reslist_ ; rl != NULL ; rl = rl->next)
	rl->res->print () ;
}

void Sos::print_coap_ret_type (l2net::l2_recv_t ret)
{
    switch (ret)
    {
	case l2net::RECV_WRONG_DEST :
	    Serial.println (F ("RECV_WRONG_DEST")) ;
	    break ;
	case l2net::RECV_WRONG_TYPE :
	    Serial.println (F ("RECV_WRONG_TYPE")) ;
	    break ;
	case l2net::RECV_OK :
	    Serial.println (F ("RECV_OK")) ;
	    break ;
	default :
	    Serial.println (F ("ERROR RECV")) ;
	    break ;
    }
}

void Sos::print_status (uint8_t status)
{
    switch (status)
    {
	case SL_COLDSTART :
	    Serial.print (F ("SL_COLDSTART")) ;
	    break ;
	case SL_WAITING_UNKNOWN :
	    Serial.print (F ("SL_WAITING_UNKNOWN")) ;
	    break ;
	case SL_WAITING_KNOWN :
	    Serial.print (F ("SL_WAITING_KNOWN")) ;
	    break ;
	case SL_RENEW :
	    Serial.print (F ("SL_RENEW")) ;
	    break ;
	case SL_RUNNING :
	    Serial.print (F ("SL_RUNNING")) ;
	    break ;
	default :
	    Serial.print (F ("???")) ;
	    Serial.print (status) ;
	    break ;
    }
}
