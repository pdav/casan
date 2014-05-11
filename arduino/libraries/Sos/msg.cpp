/**
 * @file msg.cpp
 * @brief Msg class implementation
 */

#include "msg.h"

// SOS is based on this CoAP version
#define	SOS_VERSION	1

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

#define	OPTVAL(o)	((o)->optval_ ? (o)->optval_ : (o)->staticval_)

/******************************************************************************
Constructor, destructor, operators
******************************************************************************/

/**
 * Default constructor: initialize an empty message.
 *
 * The current L2 network is associated to a message.
 *
 * @param l2 pointer to the l2 network associated to this message
 */

Msg::Msg (l2net *l2)
{
    memset (this, 0, sizeof *this) ;
    l2_ = l2 ;
}

/**
 * Copy-constructor: copy a message, including its payload and its
 * options.
 */

Msg::Msg (const Msg &m) 
{
    msgcopy (m) ;
}

/**
 * Destructor
 */

Msg::~Msg ()
{
    reset () ;
}

/**
 * Reset function: free memory, etc.
 */

void Msg::reset (void)
{
    l2net *l2 ;

    l2 = l2_ ;
    if (payload_ != NULL)
	delete payload_ ;
    if (encoded_ != NULL)
	free (encoded_) ;
    while (optlist_ != NULL)
	delete pop_option () ;
    memset (this, 0, sizeof *this) ;
    l2_ = l2 ;
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

/******************************************************************************
 * Receive message
 *
 * Methods for:
 * - receive and decode a message
 * - access informations from the received message
 */

/**
 * @brief Receive and decode a message
 *
 * Receive a message on a given L2 network, and decode it
 * according to CoAP specification if it is a valid incoming
 * message (`l2net::RECV_OK`) or if it has been truncated
 * (`l2net::RECV_TRUNCATED`). In this case, only the first part
 * of the message (excluding options and payload) is decoded.
 * This method may return `l2net::RECV_EMPTY` if no message has been
 * received, or any other value returned by the L2net-* `recv`
 * methods (such as `l2net::RECV_WRONG_TYPE` in the case of
 * Ethernet).
 *
 * @return receive status (see l2net class)
 */

l2net::l2_recv_t Msg::recv (void)
{
    l2net::l2_recv_t r ;

    reset () ;
    r = l2_->recv () ;
    if (r == l2net::RECV_OK || r == l2net::RECV_TRUNCATED)
    {
	bool trunc = (r == l2net::RECV_TRUNCATED) ;
	if (! coap_decode (l2_->get_payload (0), l2_->get_paylen (), trunc))
	    r = l2net::RECV_EMPTY ;
    }
    return r ;
}

/**
 * @brief Decode a message according to CoAP specification.
 *
 * Returns true if message decoding was successful.
 * If message has been truncated, decoding is done only for
 * CoAP header and token (and considered as a success).
 *
 * @param rbuf	L2 payload as received by the L2 network
 * @param len	Length of L2 payload
 * @param truncated true if the message has been truncated at reception
 * @return True if decoding was successful (even if truncated)
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

/**
 * @brief Encode and send a message
 *
 * Encode a message according to CoAP specification and
 * send the result to the given L2 address on the given
 * L2 network.
 *
 * Memory is allocated for the encoded message. It will
 * be freed when the object will be destroyed (the encoded
 * message is kept since it may have to be retransmitted).
 * If the encoded message does not fit in this buffer, this
 * method reports an error (false value)
 *
 * Note: the return value is the value returned by the `send`
 * method of the appropriate L2net-* class. It means that
 * the message has been sent to the network hardware, and
 * does not mean that the message has been successfully sent.
 *
 * @param dest L2 destination address
 * @return true if encoding was successfull
 */

bool Msg::send (l2addr &dest) 
{
    int success ;

    if (encoded_ == NULL)
    {
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

/**
 * @brief Encode a message according to the CoAP specification
 *
 * If message does not fit in the given buffer, an error is returned.
 *
 * @param sbuf memory allocated for the encoded message
 * @param sbuflen size of sbuf
 * @return true if encoding was successfull
 */

bool Msg::coap_encode (uint8_t sbuf [], size_t &sbuflen)
{
    uint16_t i ;
    uint16_t opt_nb ;
    uint16_t size ;
    bool success ;
    bool emulpayload ;

    /*
     * Format message, part 1 : compute message size
     */

    emulpayload = false ;		// get the true available space
    size = coap_size (emulpayload) ;

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
    else
    {
	Serial.print (F ("Message truncated on CoAP encoding")) ;
	success = false ;
    }

    return success ;
}

/**
 * @brief Compute encoded message size
 * 
 * Compute the size of the message when it will be encoded according
 * to the CoAP specification. This computation is done according to
 * token, options and payload currently associated with the message.
 * Since the end of options is marked with a 0xff byte before the
 * payload, we have to know if a payload will be added in the future
 * in order to estimate available space in the message.
 *
 * @param emulpayload true if a payload will be added in the future
 * @return estimated size of the encoded message
 */

size_t Msg::coap_size (bool emulpayload)
{
    uint16_t opt_nb ;
    size_t size ;

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
    if (paylen_ > 0 || emulpayload)
	size += 1 + paylen_ ;		// don't forget 0xff byte

    return size ;
}

/**
 * @brief Estimate the available space in the message
 *
 * Compute the available space in the message, according to L2 MTU
 * and size of message when it will be encoded. Typically used to
 * know available space for the payload.
 *
 * @return Available space in the message, or 0 if the message does
 * not fit.
 */

size_t Msg::avail_space (void)
{
    size_t size, mtu, avail ;

    size = coap_size (true) ;
    mtu = l2_->mtu () ;

    avail = (size <= mtu) ? mtu - size : 0 ;
    return avail ;
}

/*
 * Various mutators to long to be inlined: token and payload
 */

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


/******************************************************************************
 * Option management
 */

/**
 * @brief Remove the first option from the option list
 *
 * @return First option
 */

option *Msg::pop_option (void) 
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

/**
 * @brief Push an option in the option list
 *
 * The option list is kept sorted according to option values
 * in order to optimally encode CoAP options.
 */

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

/**
 * @brief Reset the option iterator
 *
 * This method resets the internal pointer (in option list) used by the
 * the option iterator (see `next_option`).
 */

void Msg::reset_next_option (void) 
{
    curopt_initialized_ = false ;
}

/**
 * @brief Option iterator
 *
 * Each call to this method will return the next element in option
 * list, thanks to an internal pointer which is advanced in this method.
 * Looping through options must start by a call to
 * `reset_next_option' before first use.
 */

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


/*
 * Copy a whole message, including payload and option list
 */

void Msg::msgcopy (const Msg &m)
{
    struct optlist *ol1, *ol2 ;

    memcpy (this, &m, sizeof m) ;

    if (payload_)
	free (payload_) ;

    payload_ = (uint8_t *) malloc (paylen_) ;
    memcpy (payload_, m.payload_, paylen_) ;

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

/**
 * @brief Returns content_format option
 *
 * This method returns the content_format option, if present,
 * or `option::cf_none` if not present.
 *
 * @return value associated with the content_format option or option::cf_none
 */

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

/**
 * @brief Set content_format option
 *
 * If the content_format option is not already present in option list,
 * this method adds the option.
 * If the reset argument is true, the content_format option value
 * is reset to the given value (otherwise, option is not modified).
 *
 * @param reset true if the existing option, if exists, must be reset
 *	to the given value
 * @param cf the new value
 */

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

/*
 * Useful for debugging purposes
 */

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
