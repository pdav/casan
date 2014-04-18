#include "msg.h"

#define	FORMAT_BYTE0(ver,type,toklen)				\
			((((unsigned int) (ver) & 0x3) << 6) |	\
			 (((unsigned int) (type) & 0x3) << 4) |	\
			 (((unsigned int) (toklen) & 0x7))	\
			 )
#define	COAP_VERSION(b)	(((b) [0] >> 6) & 0x3)
#define	COAP_TYPE(b)	(((b) [0] >> 4) & 0x3)
#define	COAP_TOKLEN(b)	(((b) [0]     ) & 0xf)
#define	COAP_CODE(b)	(((b) [1]))
#define	COAP_ID(b)	(((b) [2] << 8) | (b) [3])

Msg::Msg ()
{
    memset (this, 0, sizeof *this) ;
}

Msg::Msg (const Msg &m) 
{
    msgcopy (m) ;
}

Msg &Msg::operator= (const Msg &m) 
{
    if (this != &m)
	msgcopy (m) ;
    return *this ;
}

bool Msg::operator== (const Msg &m)
{
    return this->id_ == m.id_ ;
}

Msg::~Msg ()
{
    reset () ;
}

void Msg::reset (void)
{
    if (payload_ != NULL)
	delete payload_ ;
    if (encoded_ != NULL)
	free (encoded_) ;
    while (optlist_ != NULL)
	delete pop_option () ;
    memset (this, 0, sizeof *this) ;
}

/******************************************************************************
 * Receive message
 *
 * Methods for:
 * - receive and decode a message
 * - access informations from the received message
 */

l2_recv_t Msg::recv (l2net &l2)
{
    l2_recv_t r ;

    reset () ;
    l2_ = &l2 ;
    r = l2_->recv () ;
    if (r == L2_RECV_RECV_OK || r == L2_RECV_TRUNCATED)
    {
	bool trunc = (r == L2_RECV_TRUNCATED) ;
	if (! coap_decode (l2_->get_payload (0), l2_->get_paylen (), trunc))
	    r = L2_RECV_EMPTY ;
    }
    return r ;
}

/*
 * Returns true if message is decoding was successful
 * If message has been truncated, decoding is done only for
 * CoAP header and token (and considered as a success).
 */

bool Msg::coap_decode (uint8_t rbuf [], size_t len, bool truncated)
{
    bool success ;

    reset () ;
    success = true ;

    if (COAP_VERSION (rbuf) != SOS_VERSION)
    {
	success = false ;
    }
    else
    {
	size_t i ;
	int opt_nb ;

	type_ = COAP_TYPE (rbuf) ;
	toklen_ = COAP_TOKLEN (rbuf) ;
	code_ = COAP_CODE (rbuf) ;
	id_ = COAP_ID (rbuf) ;
	i = 4 ;

	if (toklen_ > 0)
	{
	    memcpy (token_, rbuf + i, toklen_) ;
	    i += toklen_ ;
	}

	/*
	 * Options analysis
	 */

	opt_nb = 0 ;
	while (! truncated && success && i < len && rbuf [i] != 0xff)
	{
	    int opt_delta = 0, opt_len = 0 ;
	    option o ;

	    opt_delta = (rbuf [i] >> 4) & 0x0f ;
	    opt_len   = (rbuf [i]     ) & 0x0f ;
	    i++ ;
	    switch (opt_delta)
	    {
		case 13 :
		    opt_delta = rbuf [i] + 13 ;
		    i += 1 ;
		    break ;
		case 14 :
		    opt_delta = (rbuf [i] << 8) + rbuf [i+1] + 269 ;
		    i += 2 ;
		    break ;
		case 15 :
		    success = false ;			// recv failed
		    break ;
	    }
	    opt_nb += opt_delta ;

	    switch (opt_len)
	    {
		case 13 :
		    opt_len = rbuf [i] + 13 ;
		    i += 1 ;
		    break ;
		case 14 :
		    opt_len = (rbuf [i] << 8) + rbuf [i+1] + 269 ;
		    i += 2 ;
		    break ;
		case 15 :
		    success = false ;			// recv failed
		    break ;
	    }

	    /* register option */
	    if (success)
	    {
		o.optcode (option::optcode_t (opt_nb)) ;
		o.optval ((void *)(rbuf + i), (int) opt_len) ;
		push_option (o) ;

		i += opt_len ;
	    }
	    else
	    {
		success = false ;
		Serial.print (F ("\033[31mOPTION unrecognized\033[00m")) ;
		Serial.print (F (" opt_delta=")) ;
		Serial.print (opt_delta) ;
		Serial.print (F (", opt_len=")) ;
		Serial.print (opt_len) ;
		Serial.println () ;
	    }
	}

	paylen_ = len - i ;
	if (! truncated && success && paylen_ > 0)
	{
	    if (rbuf [i] != 0xff)
	    {
		success = false ;
	    }
	    else
	    {
		i++ ;
		set_payload (rbuf + i, paylen_) ;
	    }
	}
	else paylen_ = 0 ;			// protect further operations
    }

    return success ;
}

/******************************************************************************
 * Send message
 *
 * Methods for:
 * - encode a message
 * - access informations from the received message
 */

bool Msg::send (l2net &l2, l2addr &dest) 
{
    int success ;

    if (encoded_ == NULL)
    {
	l2_ = &l2 ;
	enclen_ = l2_->mtu () ;			// exploitable size
	encoded_ = (uint8_t *) malloc (enclen_) ;
	success = coap_encode (encoded_, enclen_) ;
    }
    else success = true ;			// if msg is already encoded

    if (success)
    {
	success = l2_->send (dest, encoded_, enclen_) ;
    }
    else
    {
	free (encoded_) ;
	encoded_ = NULL ;
    }

    return success ;
}

bool Msg::coap_encode (uint8_t sbuf [], size_t &sbuflen)
{
    uint16_t i ;
    uint16_t opt_nb ;
    uint16_t size ;
    bool success ;

    /*
     * Format message, part 1 : compute message size
     */

    size = 4 + toklen_ ;

    reset_next_option () ;
    opt_nb = 0 ;
    for (option *o = next_option () ; o != NULL ; o = next_option ())
    {
	int opt_delta, opt_len ;

	size++ ;			// 1 byte for opt delta & len

	opt_delta = o->optcode_ - opt_nb ;
	if (opt_delta >= 269)		// delta >= 269 => 2 bytes
	    size += 2 ;
	else if (opt_delta >= 13)	// delta \in [13..268] => 1 byte
	    size += 1 ;
	opt_nb = o->optcode_ ;

	opt_len = o->optlen_ ;
	if (opt_len >= 269)		// len >= 269 => 2 bytes
	    size += 2 ;
	else if (opt_len >= 13)		// len \in [13..268] => 1 byte
	    size += 1 ;
	size += o->optlen_ ;
    }
    if (paylen_ > 0)
	size += 1 + paylen_ ;		// don't forget 0xff byte

    if (size <= sbuflen)		// Enough space?
    {
	success = true ;
	sbuflen = size ;

	/*
	 * Format message, part 2 : build message
	 */

	i = 0 ;

	// header
	sbuf [i++] = FORMAT_BYTE0 (SOS_VERSION, type_, toklen_) ;
	sbuf [i++] = code_ ;
	sbuf [i++] = BYTE_HIGH (id_) ;
	sbuf [i++] = BYTE_LOW  (id_) ;
	// token
	if (toklen_ > 0)
	{
	    memcpy (sbuf + i, token_, toklen_) ;
	    i += toklen_ ;
	}
	// options
	reset_next_option () ;
	opt_nb = 0 ;
	for (option *o = next_option () ; o != NULL ; o = next_option ())
	{
	    int opt_delta, opt_len ;
	    int posoptheader = i ;

	    sbuf [posoptheader] = 0 ;

	    i++ ;
	    opt_delta = int (o->optcode_) - opt_nb ;
	    if (opt_delta >= 269)		// delta >= 269 => 2 bytes
	    {
		opt_delta -= 269 ;
		sbuf [i++] = BYTE_HIGH (opt_delta) ;
		sbuf [i++] = BYTE_LOW  (opt_delta) ;
		sbuf [posoptheader] |= 0xe0 ;
	    }
	    else if (opt_delta >= 13)		// delta \in [13..268] => 1 byte
	    {
		opt_delta -= 13 ;
		sbuf [i++] = BYTE_LOW (opt_delta) ;
		sbuf [posoptheader] |= 0xd0 ;
	    }
	    else
	    {
		sbuf [posoptheader] |= (opt_delta << 4) ;
	    }
	    opt_nb = o->optcode_ ;

	    opt_len = o->optlen_ ;
	    if (opt_len >= 269)			// len >= 269 => 2 bytes
	    {
		opt_len -= 269 ;
		sbuf [i++] = BYTE_HIGH (opt_len) ;
		sbuf [i++] = BYTE_LOW  (opt_len) ;
		sbuf [posoptheader] |= 0x0e ;
	    }
	    else if (opt_len >= 13)		// len \in [13..268] => 1 byte
	    {
		sbuf [i++] = BYTE_LOW (opt_len) ;
		sbuf [posoptheader] |= 0x0d ;
	    }
	    else
	    {
		sbuf [posoptheader] |= opt_len ;
	    }
	    memcpy (sbuf + i, OPTVAL (o), o->optlen_) ;
	    i += o->optlen_ ;
	}
	// payload
	if (paylen_ > 0)
	{
	    sbuf [i++] = 0xff ;			// start of payload
	    memcpy (sbuf + i, payload_, paylen_) ;
	}
    }
    else success = false ;

    return success ;
}

uint8_t *Msg::get_payload_copy (void) 
{
    uint8_t *c = (uint8_t *) malloc (paylen_) ;
    memcpy (c, payload_, paylen_) ;
    return c ;
}

option * Msg::pop_option (void) 
{
    option *r = NULL ;
    if (optlist_ != NULL)
    {
	optlist *next ;

	r = optlist_->o ;
	next = optlist_->next ;
	delete optlist_ ;
	optlist_ = next ;
    }
    return r ;
}

void Msg::set_token (uint8_t toklen, uint8_t *token)
{
    toklen_ = toklen ;
    memcpy (token_, token, toklen) ;
}

void Msg::set_payload (uint8_t *payload, uint16_t paylen) 
{
    paylen_ = paylen ;
    if (payload_ != NULL)
	free (payload_) ;
    payload_ = (uint8_t *) malloc (paylen_) ;
    memcpy (payload_, payload, paylen_) ;
}

// push option in the sorted list of options
void Msg::push_option (option &o) 
{
    optlist *newo, *prev, *cur ;

    newo = new optlist ;
    newo->o = new option (o) ;
    prev = NULL ;
    cur = optlist_ ;

    while (cur != NULL && *(newo->o) >= *(cur->o))
    {
	prev = cur ;
	cur = cur->next ;
    }

    newo->next = cur ;
    if (prev == NULL)
	optlist_ = newo ;
    else
	prev->next = newo ;
}

void Msg::msgcopy (const Msg &m)
{
    struct optlist *ol1, *ol2 ;

    memcpy (this, &m, sizeof m) ;

    if (payload_)
	free (payload_) ;
    payload_ = ((Msg) m).get_payload_copy () ;

    enclen_ = 0 ;
    if (encoded_ != NULL)
	free (encoded_) ;
    encoded_ = NULL ;

    optlist_ = NULL ;
    curopt_ = NULL ;
    curopt_initialized_ = false ;

    ol1 = NULL ;
    for (ol2 = m.optlist_ ; ol2 != NULL ; ol2 = ol2->next)
    {
	optlist *newo ;

	newo = new optlist ;
	newo->o = new option (*ol2->o) ;
	newo->next = NULL ;
	if (ol1 == NULL)
	    optlist_ = newo ;
	else
	    ol1->next = newo ;
	ol1 = newo ;
    }
}

void Msg::reset_next_option (void) 
{
    curopt_initialized_ = false ;
}

option *Msg::next_option (void) 
{
    option *o ;
    if (! curopt_initialized_)
    {
	curopt_ = optlist_ ;
	curopt_initialized_ = true ;
    }
    if (curopt_ == NULL)
    {
	o = NULL ;
	curopt_initialized_ = false ;
    }
    else
    {
	o = curopt_->o ;
	curopt_ = curopt_->next ;
    }
    return o ;
}

option::content_format Msg::content_format (void)
{
    optlist *ol ;
    option::content_format cf ;
    
    cf = option::cf_none ;		// not found by default ;
    for (ol = optlist_ ; ol != NULL ; ol = ol->next)
    {
	if (ol->o->optcode () == option::MO_Content_Format)
	{
	    cf = (option::content_format) ol->o->optval () ;
	    break ;
	}
    }
    return cf ;
}

void Msg::content_format (bool reset, option::content_format cf)
{
    optlist *ol ;

    // look for the ContentFormat option
    for (ol = optlist_ ; ol != NULL ; ol = ol->next)
	if (ol->o->optcode () == option::MO_Content_Format)
	    break ;

    if (ol != NULL)			// found
    {
	if (reset)			// reset it to the new value?
	    ol->o->optval (cf) ;	// yes
    }
    else				// not found: add this option
    {
	option *ocf ;

	ocf = new option (option::MO_Content_Format, option::cf_text_plain) ;
	push_option (*ocf) ;
	delete ocf ;
    }
}

void Msg::print (void) 
{
    char *p ;
    int len ;

    Serial.print (F ("\033[36mmsg\033[00m\tid=")) ;
    Serial.print (get_id ()) ;
    Serial.print (F (", type=")) ;
    switch (get_type ()) {
	case COAP_TYPE_CON : Serial.print (F ("CON")) ; break ;
	case COAP_TYPE_NON : Serial.print (F ("NON")) ; break ;
	case COAP_TYPE_ACK : Serial.print (F ("ACK")) ; break ;
	case COAP_TYPE_RST : Serial.print (F ("RST")) ; break ;
	default : Serial.print (F ("\033[31mERROR\033[00m")) ;
    }
    Serial.print (F (", code=")) ;
    Serial.print (get_code () >> 5, HEX) ;
    Serial.print (".") ;
    Serial.print (get_code () & 0x1f, HEX) ;
    Serial.print (F (", toklen=")) ;
    Serial.print (get_toklen ()) ;

    if (get_toklen () > 0) {
	Serial.print (F (", token=")) ;
	uint8_t *token = get_token () ;
	for (int i = 0 ; i < get_toklen () ; i++)
	    Serial.print (token [i], HEX) ;
	Serial.println () ;
    }

    p = (char *) get_payload () ;
    len = get_paylen () ;
    Serial.print (F (", paylen=")) ;
    Serial.print (len) ;
    Serial.print (F (", payload=")) ;
    for (int i = 0 ; i < len ; i++)
	Serial.print (p [i]) ;
    Serial.println () ;

    reset_next_option () ;
    for (option *o = next_option () ; o != NULL ; o = next_option ()) {
	o->print () ;
    }
}
