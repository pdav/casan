#include "sos.h"

/*
 * SOS main class
 *
 * Note: this class supports at most one master on the current L2
 *   network. If there are more than one master, behaviour is not
 *   guaranteed.
 */

#define	SOS_NAMESPACE1		".well-known"
#define	SOS_NAMESPACE2		"sos"
#define	SOS_HELLO		"hello=%ld"
#define	SOS_DISCOVER_SLAVEID	"slave=%ld"
#define	SOS_DISCOVER_MTU	"mtu=%d"
#define	SOS_ASSOC		"assoc=%ld"

#define	SOS_BUF_LEN		50	// > sizeof hello=.../slave=..../etc


static struct
{
    const char *path ;
    int len ;
} sos_namespace [] =
{
    {  SOS_NAMESPACE1, sizeof SOS_NAMESPACE1 - 1 },
    {  SOS_NAMESPACE2, sizeof SOS_NAMESPACE2 - 1 },
} ;

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

    rmanager_ = new Rmanager () ;
    retrans_ = new Retrans (&master_) ;
    status_ = SL_COLDSTART ;
}

void Sos::reset_master (void)
{
    if (master_ != NULL)
	delete master_ ;
    master_ = NULL ;
    hlid_ = -1 ;

    Serial.println (F ("Master reset to broadcast address")) ;
}

// a cannot be a NULL pointer
bool Sos::same_master (l2addr *a)
{
    return master_ != NULL && *a == *master_ ;
}

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

void Sos::register_resource (Resource *r)
{
    rmanager_->add_resource (r) ;
}

// TODO : we need to restart all the application,
// delete all the history of the exchanges
// XXX: not used at this time
void Sos::reset (void)
{
    status_ = SL_COLDSTART ;
    curid_ = 1 ;
    rmanager_->reset () ;
    retrans_->reset () ;
    reset_master () ;
}

/*
 * Main SOS loop
 * This method must be called regularly (typically in the loop function
 * of the Arduino framework) in order to process SOS events.
 */

void Sos::loop ()
{
    Msg in ;
    Msg out ;
    l2_recv_t ret ;
    uint8_t oldstatus ;
    long int hlid ;
    l2addr *srcaddr ;

    oldstatus = status_ ;		// keep old value for debug display
    sync_time (curtime) ;		// get current time
    retrans_->loop (*l2_, curtime) ;	// check needed retransmissions

    srcaddr = NULL ;

    ret = in.recv (*l2_) ;		// get received message
    if (ret == L2_RECV_RECV_OK)
	srcaddr = l2_->get_src () ;	// get a new address

    switch (status_)
    {
	case SL_COLDSTART :
	    send_discover (out) ;
	    twait_.init (curtime) ;
	    status_ = SL_WAITING_UNKNOWN ;
	    break ;

	case SL_WAITING_UNKNOWN :
	    if (ret == L2_RECV_RECV_OK)
	    {
		retrans_->check_msg_received (in) ;

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
		    else Serial.println (F ("\033[31mignored ctl msg\033[00m")) ;
		}
	    }

	    if (status_ == SL_WAITING_UNKNOWN && twait_.next (curtime))
		send_discover (out) ;

	    break ;

	case SL_WAITING_KNOWN :
	    if (ret == L2_RECV_RECV_OK)
	    {
		retrans_->check_msg_received (in) ;

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
		    else Serial.println (F ("\033[31mignored ctl msg\033[00m")) ;
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
	    if (ret == L2_RECV_RECV_OK)
	    {
		retrans_->check_msg_received (in) ;

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
		    else Serial.println (F ("\033[31mignored ctl msg\033[00m")) ;
		}
		else		// request for a normal resource
		{
		    // deduplicate () ;
		    rmanager_->request_resource (in, out) ;
		    out.send (*l2_, *master_) ;
		}
	    }
	    else if (ret == L2_RECV_TRUNCATED)
	    {
		Serial.println (F ("\033[36mRequest too large\033[00m")) ;
		out.set_type (COAP_TYPE_ACK) ;
		out.set_id (in.get_id ()) ;
		out.set_token (in.get_toklen (), in.get_token ()) ;
		option o (option::MO_Size1, l2_->mtu ()) ;
		out.push_option (o) ;
		out.set_code (COAP_RETURN_CODE (4,13)) ;
		out.send (*l2_, *master_) ;
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
	    PRINT_DEBUG_DYNAMIC (status_) ;
	    break ;
    }

    if (oldstatus != status_)
    {
	Serial.print (F ("Status: ")) ;
	print_status (oldstatus) ;
	Serial.print (F (" -> ")) ;
	print_status (status_) ;
	Serial.println () ;
    }

    if (srcaddr != NULL)
	delete srcaddr ;
}

void Sos::send_assoc_answer (Msg &in, Msg &out)
{
    l2addr *dest ;

    dest = l2_->get_src () ;

    // send back an acknowledgement message
    out.set_type (COAP_TYPE_ACK) ;
    out.set_code (COAP_RETURN_CODE (2, 5)) ;
    out.set_id (in.get_id ()) ;

    // will get the resources and set them in the payload in the right format
    rmanager_->get_resource_list (out) ;

    // send the packet
    if (! out.send (*l2_, *dest))
	Serial.println (F ("Error: cannot send the assoc answer message")) ;

    delete dest ;
}

/**********************
 * SOS control messages
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
	    if (memcmp (sos_namespace [i].path, o->val (), o->optlen ()))
		return false ;
	    i++ ;
	}
    }
    m.reset_next_option () ;
    if (i != NTAB (sos_namespace))
	return false ;

    return true ;
}

void Sos::mk_ctl_msg (Msg &m)
{
    int i ;

    for (i = 0 ; i < NTAB (sos_namespace) ; i++)
    {
	option path (option::MO_Uri_Path, (void *) sos_namespace [i].path,
					sos_namespace [i].len) ;
	m.push_option (path) ;
    }
}

void Sos::send_discover (Msg &m)
{
    char tmpstr [SOS_BUF_LEN] ;
    l2addr *dest ;

    Serial.println (F ("Sending Discover")) ;

    m.reset () ;
    m.set_id (curid_++) ;
    m.set_type (COAP_TYPE_NON) ;
    m.set_code (COAP_CODE_POST) ;
    mk_ctl_msg (m) ;

    snprintf (tmpstr, sizeof tmpstr, SOS_DISCOVER_SLAVEID, slaveid_) ;
    option o1 (option::MO_Uri_Query, tmpstr, strlen (tmpstr)) ;
    m.push_option (o1) ;

    snprintf (tmpstr, sizeof tmpstr, SOS_DISCOVER_MTU, l2_->mtu ()) ;
    option o2 (option::MO_Uri_Query, tmpstr, strlen (tmpstr)) ;
    m.push_option (o2) ;

    dest = (master_ != NULL) ? master_ : l2_->bcastaddr () ;

    m.send (*l2_, *dest) ;
}

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
		if (sscanf ((const char *) o->val (), SOS_ASSOC, &n) == 1)
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

// check if a hello msg is received
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
		if (sscanf ((const char *) o->val (), SOS_HELLO, &hlid) == 1)
		    found = true ;
	    }
	}
    }

    return found ;
}

/*****************************************
 * useful display of pieces of information
 */

void Sos::print_coap_ret_type (l2_recv_t ret)
{
    switch (ret)
    {
	case L2_RECV_WRONG_DEST :
	    PRINT_DEBUG_STATIC ("L2_RECV_WRONG_DEST") ;
	    break ;
	case L2_RECV_WRONG_ETHTYPE :
	    PRINT_DEBUG_STATIC ("L2_RECV_WRONG_ETHTYPE") ;
	    break ;
	case L2_RECV_RECV_OK :
	    PRINT_DEBUG_STATIC ("L2_RECV_RECV_OK") ;
	    break ;
	default :
	    PRINT_DEBUG_STATIC ("ERROR RECV") ;
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

void Sos::print_resources (void)
{
    rmanager_->print () ;
}
