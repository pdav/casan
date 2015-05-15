/**
 * @file msg.cc
 * @brief msg class implementation
 */

#include <iostream>
#include <cstring>
#include <cstdio>
#include <vector>

#include "global.h"

#include "casan.h"
#include "l2.h"
#include "msg.h"
#include "slave.h"
#include "utils.h"
#include "byte.h"

namespace casan {

int msg::global_message_id = 1 ;

// reset pointer and length
#define	RESET_PL(p,l)	do {					\
			    if (p != nullptr)			\
			    {					\
				delete [] p ;			\
				p = nullptr ;			\
				l = 0 ;				\
			    }					\
			} while (false)				// no ";"
// reset encoded message
#define	RESET_BINARY	do {					\
			    RESET_PL (msg_, msglen_) ;		\
			} while (false)				// no ";"
// reset all pointers
#define	RESET_POINTERS	do {					\
			    RESET_PL (msg_, msglen_) ;		\
			    RESET_PL (payload_, paylen_) ;	\
			    if (reqrep_ != nullptr)		\
				reqrep_->reqrep_ = nullptr ;	\
			    reqrep_ = nullptr ;			\
			    optlist_.clear () ;			\
			} while (false)				// no ";"
// reset all values and pointers (but don't deallocate them)
#define	RESET_VALUES	do {					\
			    peer_ = nullptr ;			\
			    reqrep_ = nullptr ;			\
			    msg_ = nullptr ; msglen_ = 0 ;	\
			    payload_ = nullptr ; paylen_ = 0 ;	\
			    toklen_ = 0 ; ntrans_ = 0 ;		\
			    timeout_ = duration_t (0) ;		\
			    next_timeout_ = std::chrono::system_clock::time_point::max () ; \
			    expire_ = std::chrono::system_clock::time_point::max () ; \
			    pktype_ = PK_NONE ;			\
			    casantype_ = CASAN_UNKNOWN ;	\
			    id_ = 0 ;				\
			} while (false)				// no ";"
#define	STOP_TRANSMIT	do {					\
			    ntrans_ = MAX_RETRANSMIT ;		\
			} while (false)				// no ";"

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

#define	ALLOC_COPY(f,m,l)	do {				\
				    f = new byte [(l)] ;	\
				    std::memcpy (f, (m), (l)) ;	\
				} while (false)			// no ";"
// add a nul byte to ease string operations
#define	ALLOC_COPYNUL(f,m,l)	do {				\
				    f = new byte [(l) + 1] ;	\
				    std::memcpy (f, (m), (l)) ;	\
				    f [(l)]=0 ;			\
				} while (false)			// no ";"
#define	OPTVAL(o)	((o).optval_ ? (o).optval_ : (o).staticval_)

/**
 * @brief Default constructor: initialize an empty message.
 */

msg::msg ()
{
    RESET_VALUES ;
}

/**
 * @brief Copy-constructor: copy a message, including its payload and its
 * options.
 */

msg::msg (const msg &m)
{
    *this = m ;
    if (msg_ != nullptr)
	ALLOC_COPY (msg_, m.msg_, msglen_) ;
    if (payload_)
	ALLOC_COPYNUL (payload_, m.payload_, paylen_) ;
}

/**
 * @brief Copy-assignement constructor: copy a message, including its
 * payload and its options.
 */

msg &msg::operator= (const msg &m)
{
    if (this != &m)
    {
	RESET_POINTERS ;
	*this = m ;
	if (msg_ != nullptr)
	    ALLOC_COPY (msg_, m.msg_, msglen_) ;
	if (payload_)
	    ALLOC_COPYNUL (payload_, m.payload_, paylen_) ;
    }
    return *this ;
}

/**
 * @brief Destructor
 */

msg::~msg ()
{
    RESET_POINTERS ;
}

/**
 * @brief Dump message attributes for debugging purposes
 */

std::ostream& operator<< (std::ostream &os, const msg &m)
{
    char buf1 [MAXBUF], buf2 [MAXBUF] ;
    std::time_t nt ;

    nt = std::chrono::system_clock::to_time_t (m.expire_) ;
    strftime (buf1, sizeof buf1, "%T", std::localtime (&nt)) ;

    nt = std::chrono::system_clock::to_time_t (m.next_timeout_) ;
    strftime (buf2, sizeof buf2, "%T", std::localtime (&nt)) ;

    os << "msg <id=" << m.id_
	<< ", toklen=" << m.toklen_
	<< ", paylen=" << m.paylen_
	<< ", ntrans=" << m.ntrans_
	<< ", expire=" << buf1
	<< ", next_timeout=" << buf2
	<< ">" ;

    return os ;
}

/******************************************************************************
 * Operators
 */

// Only for received messages
int msg::operator == (msg &m)
{
    int r ;

    r = 0 ;
    if (msg_ != nullptr && m.msg_ != nullptr && msglen_ == m.msglen_)
	r = std::memcmp (msg_, m.msg_, msglen_) == 0 ;
    return r ;
}

/******************************************************************************
 * Receive message
 */

/**
 * @brief Receive and decode a message
 *
 * Receive a message on a given L2 network, and decode it
 * according to CoAP specification if it is a valid incoming
 * message.
 *
 * @param l2 L2 network access
 * @return receive status (see l2net class)
 */

l2addr *msg::recv (l2net *l2)
{
    l2addr *a ;
    int len ;

    /*
     * Reset message object to a known state
     */

    RESET_POINTERS ;
    RESET_VALUES ;

    /*
     * Read message from L2 network
     */

    len = l2->mtu () ;
    msg_ = new byte [len] ;
    pktype_ = l2->recv (&a, msg_, &len) ; 	// create a l2addr *a
    msglen_ = len ;

    if (! ((pktype_ == PK_ME || pktype_ == PK_BCAST) && coap_decode ()))
    {
	/*
	 * Packet reception failed, not addressed to me, or not a CASAN packet
	 */

	delete a ;			// remove address created by l2->recv ()
	a = 0 ;
    }

#ifdef DEBUG
    if (a)
    {
	const char *p ;
	if (pktype_ == PK_ME) p = "me" ;
	if (pktype_ == PK_BCAST) p = "bcast" ;

	D (D_MESSAGE, "VALID RECV -> " << p << ", id=" << id_ << ", len=" << msglen_) ;
    }
    else D (D_MESSAGE, "INVALID RECV pkt=" << pktype_ << ", len=" << len) ;
#endif

    return a ;
}

/**
 * @brief Decode a message according to CoAP specification.
 *
 * @return True if decoding was successful
 */

bool msg::coap_decode (void)
{
    bool success ;

    if (COAP_VERSION (msg_) != CASAN_VERSION)
    {
	success = false ;
    }
    else
    {
	int i ;
	int opt_nb ;

	success = true ;
	i = 0 ;

	type_ = msgtype_t (COAP_TYPE (msg_ + i)) ;
	toklen_ = COAP_TOKLEN (msg_ + i) ;
	code_ = COAP_CODE (msg_ + i) ;
	id_ = COAP_ID (msg_ + i) ;
	i += 4 ;

	if (toklen_ > 0)
	{
	    std::memcpy (token_, msg_ + i, toklen_) ;
	    i += toklen_ ;
	}

	/*
	 * Options analysis
	 */

	opt_nb = 0 ;
	while (success && i < msglen_ && msg_ [i] != 0xff)
	{
	    int opt_delta, opt_len ;
	    option o ;

	    opt_delta = (msg_ [i] >> 4) & 0x0f ;
	    opt_len   = (msg_ [i]     ) & 0x0f ;
	    i++ ;
	    switch (opt_delta)
	    {
		case 13 :
		    opt_delta = msg_ [i] + 13 ;
		    i += 1 ;
		    break ;
		case 14 :
		    opt_delta = (msg_ [i] << 8) + msg_ [i+1] + 269 ;
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
		    opt_len = msg_ [i] + 13 ;
		    i += 1 ;
		    break ;
		case 14 :
		    opt_len = (msg_ [i] << 8) + msg_ [i+1] + 269 ;
		    i += 2 ;
		    break ;
		case 15 :
		    success = false ;			// recv failed
		    break ;
	    }

	    /* register option */
	    if (success)
	    {
		D (D_OPTION, "OPTION opt=" << opt_nb << ", len=" << opt_len) ;
		o.optcode (option::optcode_t (opt_nb)) ;
		o.optval ((void *)(msg_ + i), opt_len) ;
		pushoption (o) ;

		i += opt_len ;
	    }
	    else
		D (D_OPTION, "OPTION unrecognized") ;
	}

	paylen_ = msglen_ - i - 1 ;
	if (success && paylen_ > 0)
	{
	    if (msg_ [i] != 0xff)
	    {
		success = false ;
	    }
	    else
	    {
		i++ ;
		ALLOC_COPYNUL (payload_, msg_ + i, paylen_) ;
	    }
	}
	else paylen_ = 0 ;			// protect further operations
    }

    return success ;
}

/******************************************************************************
 * Send message
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
 * @return number of bytes sent
 */

int msg::send (void)
{
    int r ;

    if (msg_ == nullptr)
	coap_encode () ;

    D (D_MESSAGE, "TRANSMIT id=" << id_ << " ntrans_=" << ntrans_) ;
    r = peer_->l2 ()->send (peer_->addr (), msg_, msglen_) ;
    if (r == -1)
    {
	std::cout << "ERREUR \n" ;
    }
    else
    {
	int maxlat = peer_ ->l2 ()->maxlatency () ;

	/*
	 * Timers for reliable messages
	 */

	switch (type_)
	{
	    case MT_CON :
		if (ntrans_ == 0)
		{
		    long int nmilli ;
		    int r ;

		    /*
		     * initial timeout should be \in
		     *	[ACK_TIMEOUT ... ACK_TIMEOUT * ACK_RANDOM_FACTOR] (1)
		     *
		     * Let's name i = initial timeout, t = ACK_TIMEOUT and
		     * f = ACK_RANDOM_FACTOR
		     * (1)	<==> t <= i < t*f
		     *		<==> 1 <= i/t < f
		     *		<==> 0 <= (i/t) - 1 < f-1
		     *		<==> 0 <= ((i/t) - 1) * 1000 < (f-1)*1000
		     * So, we take a pseudo-random number r between 0 and (f-1)*1000
		     *		r = ((i/t) - 1) * 1000
		     * and compute i = t(r/1000 + 1) = t*(r + 1)/1000
		     */

		    r = random_value (int ((ACK_RANDOM_FACTOR - 1.0) * 1000)) ;
		    nmilli = ACK_TIMEOUT * (r + 1) ;
		    nmilli = nmilli / 1000 ;
		    timeout_ = duration_t (nmilli) ;
		    expire_ = DATE_TIMEOUT_MS (EXCHANGE_LIFETIME (maxlat)) ;
		}
		else
		{
		    timeout_ *= 2 ;
		}
		next_timeout_ = std::chrono::system_clock::now () + timeout_ ;

		ntrans_++ ;
		break ;
	    case MT_NON :
		STOP_TRANSMIT ;
		expire_ = DATE_TIMEOUT_MS (NON_LIFETIME (maxlat)) ;
		break ;
	    case MT_ACK :
	    case MT_RST :
		/*
		 * Non reliable messages : arbitrary set the retransmission
		 * counter in order to skip further retransmissions.
		 */

		STOP_TRANSMIT ;
		expire_ = DATE_TIMEOUT_MS (MAX_RTT (maxlat)) ;	// arbitrary
		break ;
	    default :
		std::cout << "Can't happen (msg type == " << type_ << ")\n" ;
		break ;
	}
    }
    return r ;
}

/**
 * @brief Encode a message according to the CoAP specification
 */

void msg::coap_encode (void)
{
    int i ;
    int opt_nb ;

    /*
     * Format message, part 1 : compute message size
     */

    msglen_ = 4 + toklen_ ;

    optlist_.sort () ;			// sort option list
    opt_nb = 0 ;
    for (auto &o : optlist_)
    {
	int opt_delta, opt_len ;

	msglen_++ ;			// 1 byte for opt delta & len

	opt_delta = o.optcode_ - opt_nb ;
	if (opt_delta >= 269)		// delta >= 269 => 2 bytes
	    msglen_ += 2 ;
	else if (opt_delta >= 13)	// delta \in [13..268] => 1 byte
	    msglen_ += 1 ;
	opt_nb = o.optcode_ ;

	opt_len = o.optlen_ ;
	if (opt_len >= 269)		// len >= 269 => 2 bytes
	    msglen_ += 2 ;
	else if (opt_len >= 13)		// len \in [13..268] => 1 byte
	    msglen_ += 1 ;
	msglen_ += o.optlen_ ;
    }
    if (paylen_ > 0)
	msglen_ += 1 + paylen_ ;	// don't forget 0xff byte

    /*
     * Format message, part 2 : compute a default id
     */

    if (id_ == 0)
    {
	id_ = global_message_id++ ;
	if (global_message_id > 0xffff)
	    global_message_id = 1 ;
    }

    /*
     * Format message, part 3 : build message
     */

    msg_ = new byte [msglen_] ;

    i = 0 ;

    // header
    msg_ [i++] = FORMAT_BYTE0 (CASAN_VERSION, type_, toklen_) ;
    msg_ [i++] = code_ ;
    msg_ [i++] = BYTE_HIGH (id_) ;
    msg_ [i++] = BYTE_LOW  (id_) ;
    // token
    if (toklen_ > 0)
    {
	std::memcpy (msg_ + i, token_, toklen_) ;
	i += toklen_ ;
    }
    // options
    opt_nb = 0 ;
    for (auto &o : optlist_)
    {
	int opt_delta, opt_len ;
	int posoptheader = i ;

	msg_ [posoptheader] = 0 ;

	i++ ;
	opt_delta = int (o.optcode_) - opt_nb ;
	if (opt_delta >= 269)		// delta >= 269 => 2 bytes
	{
	    opt_delta -= 269 ;
	    msg_ [i++] = BYTE_HIGH (opt_delta) ;
	    msg_ [i++] = BYTE_LOW  (opt_delta) ;
	    msg_ [posoptheader] |= 0xe0 ;
	}
	else if (opt_delta >= 13)	// delta \in [13..268] => 1 byte
	{
	    opt_delta -= 13 ;
	    msg_ [i++] = BYTE_LOW (opt_delta) ;
	    msg_ [posoptheader] |= 0xd0 ;
	}
	else
	{
	    msg_ [posoptheader] |= (opt_delta << 4) ;
	}
	opt_nb = o.optcode_ ;

	opt_len = o.optlen_ ;
	if (opt_len >= 269)		// len >= 269 => 2 bytes
	{
	    opt_len -= 269 ;
	    msg_ [i++] = BYTE_HIGH (opt_len) ;
	    msg_ [i++] = BYTE_LOW  (opt_len) ;
	    msg_ [posoptheader] |= 0x0e ;
	}
	else if (opt_len >= 13)		// len \in [13..268] => 1 byte
	{
	    msg_ [i++] = BYTE_LOW (opt_len) ;
	    msg_ [posoptheader] |= 0x0d ;
	}
	else
	{
	    msg_ [posoptheader] |= opt_len ;
	}
	std::memcpy (msg_ + i, OPTVAL (o), o.optlen_) ;
	i += o.optlen_ ;
    }
    // payload
    if (paylen_ > 0)
    {
	msg_ [i++] = 0xff ;			// start of payload
	std::memcpy (msg_ + i, payload_, paylen_) ;
    }

    ntrans_ = 0 ;
}

/**
 * @brief Stop retransmissions for this message
 *
 * When an answer has been received, there is no longer need to
 * retransmit the message. Fake the retransmit counter to a high
 * value, let the expiration process (send thread, see the
 * casan::sender_thread method) remove the message from the
 * retransmit queue.
 */

void msg::stop_retransmit (void)
{
    STOP_TRANSMIT ;
}

/******************************************************************************
 * Mutators
 */

void msg::peer (slave *p)
{
    peer_ = p ;
}

void msg::token (void *data, int len)
{
    std::memcpy (token_, data, len) ;
    toklen_ = len ;
    RESET_BINARY ;
}

void msg::id (int id)
{
    id_ = id ;
    RESET_BINARY ;
}

void msg::type (msgtype_t type)
{
    type_ = type ;
    RESET_BINARY ;
}

void msg::code (int code)
{
    code_ = code ;
    RESET_BINARY ;
}

void msg::payload (void *data, int len)
{
    if (payload_)
	delete [] payload_ ;

    ALLOC_COPYNUL (payload_, data, len) ;
    paylen_ = len ;
    RESET_BINARY ;
}

/**
 * @brief Push an option in the option list
 */

void msg::pushoption (option &o)
{
    optlist_.push_back (o) ;
}

/**
 * @brief Set the waiter for this message
 */

void msg::wt (waiter *w)
{
    waiter_ = w ;
}

/**
 * @brief Returns the Max-Age option (in seconds) or -1
 */

long int msg::max_age (void)
{
    long int ma = -1 ;

    for (auto &o : optlist_)
    {
	if (o.optcode_ == option::MO_Max_Age)
	{
	    ma = o.optval () ;
	    break ;
	}
    }
    return ma ;
}

/**
 * @brief Check if two request messages match for caching
 *
 * This method checks if a request message matches the current message
 * for caching (see CoAP spec, 5.6):
 * - request method match
 * - all options match, except those marked NoCacheKey (5.4) or recognized by
 *	the cache
 */

bool msg::cache_match (msgptr_t m)
{
    bool r ;

    if (type_ != m->type_)
	r = false ;
    else
    {
	bool theend ;

	r = true ;

	/*
	 * Don't assume that each option list is already sorted
	 */

	optlist_.sort () ;
	m->optlist_.sort () ;

	/*
	 * Traverse the option list
	 */

	auto ol1 = optlist_.begin () ;
	auto ol2 = m->optlist_.begin () ;

	do
	{
	    theend = false ;

	    /* Skip the NoCacheKey options */

	    while (ol1 != optlist_.end () && ol1->nocachekey ())
		ol1++ ;
	    while (ol2 != m->optlist_.end () && ol2->nocachekey ())
		ol2++ ;

	    /* Stop if one iterator is at the end  */

	    if (ol1 == optlist_.end () && ol2 == m->optlist_.end ())
	    {
		theend = true ;
		r = true ;		// both at the end: success!
	    }
	    else if (ol1 == optlist_.end () || ol2 == m->optlist_.end ())
	    {
		theend = true ;
		r = false ;		// only one at the end: fail
	    }
	    else
	    {
		if (*ol1 == *ol2)	// we continue
		{
		    ol1++ ;
		    ol2++ ;
		}
		else
		{
		    theend = true ;
		    r = false ;		// no match: fail
		}
	    }
	} while (! theend) ;
    }
    return r ;
}


/**
 * @brief Mutually link a reply and a request messages
 *
 * This function is a class function rather than a object method, because
 * the current object is not a `shared_ptr<msg>` (`this` is a `msg *` and
 * not a `shared_ptr<msg>`).
 */

void msg::link_reqrep (msgptr_t m1, msgptr_t m2)
{
    if (m2 == nullptr)
    {
	// unlink peer message if it exists
	if (m1->reqrep_ != nullptr)
	    m2->reqrep_->reqrep_ = nullptr ;
	// unlink current message
	m1->reqrep_ = nullptr ;
    }
    else
    {
	// link messages
	m1->reqrep_ = m2 ;
	m2->reqrep_ = m1 ;
    }
}

/******************************************************************************
 * Accessors
 */

slave *msg::peer (void)
{
    return peer_ ;
}

pktype_t msg::pktype (void)
{
    return pktype_ ;
}

void *msg::token (int *toklen)
{
    *toklen = toklen_ ;
    return token_ ;
}

int msg::id (void)
{
    return id_ ;
}

msg::msgtype_t msg::type (void)
{
    return type_ ;
}

int msg::code (void)
{
    return code_ ;
}

int msg::msglen (void)
{
    return msglen_ ;
}

int msg::paylen (void)
{
    return paylen_ ;
}

void *msg::payload (int *paylen)
{
    *paylen = paylen_ ;
    return payload_ ;
}

/**
 * @brief return the linked (request/reply)  message
 *
 * @return the other message or NULL if the message has no peer message
 */

msgptr_t msg::reqrep (void)
{
    return reqrep_ ;
}

/**
 * @brief Remove the first option from the option list
 *
 * @return First option
 */

option msg::popoption (void)
{
    option o ;

    o = optlist_.front () ;
    optlist_.pop_front () ;
    return o ;
}

/**
 * @brief Reset the option iterator
 *
 * This method resets the internal pointer (in option list) used by the
 * the option iterator (see `next_option`).
 */

void msg::option_reset_iterator (void)
{
    optiter_ = optlist_.begin () ;
}

/**
 * @brief Return the next option in the option list
 *
 * This method uses the internal pointer (in option list) used by the
 * the option iterator (see `option_reset_iterator`).
 */

option *msg::option_next (void)
{
    option *o ;

    if (optiter_ == optlist_.end ())
	o = nullptr ;
    else
    {
	o = & (*optiter_) ;
	optiter_++ ;
    }
    return o ;
}

/**
 * @brief Returns the waiter for this message
 */

waiter *msg::wt (void)
{
    return waiter_ ;
}

/******************************************************************************
 * CASAN control messages
 */

#define	CASAN_NAMESPACE1	".well-known"
#define	CASAN_NAMESPACE2	"casan"
#define	CASAN_HELLO		"hello=%ld"
#define	CASAN_SLAVE		"slave=%ld"
#define	CASAN_MTU		"mtu=%ld"
#define	CASAN_TTL		"ttl=%ld"

static struct
{
    const char *path ;
    int len ;
}
casan_namespace [] =
{
    {  CASAN_NAMESPACE1, sizeof CASAN_NAMESPACE1 - 1 },
    {  CASAN_NAMESPACE2, sizeof CASAN_NAMESPACE2 - 1 },
} ;

bool msg::is_casan_ctl_msg (void)
{
    int i = 0;
    bool r = true ;

    for (auto &o : optlist_)
    {
	if (o.optcode_ == option::MO_Uri_Path)
	{
	    r = false ;
	    if (i >= NTAB (casan_namespace))
		break ;
	    if (casan_namespace [i].len != o.optlen_)
		break ;
	    if (std::memcmp (casan_namespace [i].path, OPTVAL (o), o.optlen_))
		break ;
	    r = true ;
	    i++ ;
	}
    }
    if (r && i == NTAB (casan_namespace))
	r = true ;
    else
	r = false ;
    D (D_MESSAGE, "It's " << (r ? "" : "not") << "a control message") ;
    return r ;
}

bool msg::is_casan_discover (slaveid_t &sid, int &mtu)
{
    sid = 0 ;
    mtu = 0 ;

    if (type_ == MT_NON && code_ == MC_POST && is_casan_ctl_msg ())
    {
	for (auto &o : optlist_)
	{
	    if (o.optcode_ == option::MO_Uri_Query)
	    {
		long int n ;

		// we benefit from the added nul byte at the end of optval
		if (std::sscanf ((char *) OPTVAL (o), CASAN_SLAVE, &n) == 1)
		{
		    sid = n ;
		    // continue, just in case there are other query strings
		}
		else if (std::sscanf ((char *) OPTVAL (o), CASAN_MTU, &n) == 1)
		{
		    mtu = n ;
		    // continue, just in case there are other query strings
		}
		else
		{
		    sid = 0 ;
		    break ;
		}
	    }
	}
    }
    if (sid > 0 && mtu >= 0)
	casantype_ = CASAN_DISCOVER ;
    return casantype_ == CASAN_DISCOVER ;
}

bool msg::is_casan_associate (void)
{
    bool found = false ;

    if (casantype_ == CASAN_UNKNOWN)
    {
	if (type_ == MT_CON && code_ == MC_POST && is_casan_ctl_msg ())
	{
	    for (auto &o : optlist_)
	    {
		if (o.optcode_ == option::MO_Uri_Query)
		{
		    long int n ;

		    // we benefit from the added nul byte at the end of optval
		    if (std::sscanf ((char *) OPTVAL (o), CASAN_TTL, &n) == 1)
		    {
			found = true ;
			// continue, just in case there are other query strings
		    }
		    else if (std::sscanf ((char *) OPTVAL (o), CASAN_MTU, &n) == 1)
		    {
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
	    if (found)
		casantype_ = CASAN_ASSOC_REQUEST ;
	}
    }
    return casantype_ == CASAN_ASSOC_REQUEST ;
}

msg::casantype_t msg::casan_type (void)
{
    return casan_type (true) ;
}

msg::casantype_t msg::casan_type (bool checkreqrep)
{
    if (casantype_ == CASAN_UNKNOWN)
    {
	slaveid_t sid ;
	int mtu ;

	if (! is_casan_discover (sid, mtu) && ! is_casan_associate ())
	{
	    if (checkreqrep && reqrep_ != nullptr)
	    {
		casantype_t st = reqrep_->casan_type (false) ;
		if (st == CASAN_ASSOC_REQUEST)
		    casantype_ = CASAN_ASSOC_ANSWER ;
	    }
	}
	if (casantype_ == CASAN_UNKNOWN)
	    casantype_ = CASAN_NONE ;
	if (casantype_ != CASAN_NONE)
	    D (D_MESSAGE, "CTL MSG " << casantype_) ;
    }
    return casantype_ ;
}

void msg::add_path_ctl (void)
{
    int i ;

    for (i = 0 ; i < NTAB (casan_namespace) ; i++)
    {
	option o (option::MO_Uri_Path,
			(void *) casan_namespace [i].path,
			casan_namespace [i].len) ;
	pushoption (o) ;
    }
}

void msg::mk_ctl_hello (long int hid)
{
    char buf [MAXBUF] ;

    bzero(buf, MAXBUF);

    add_path_ctl () ;
    snprintf (buf, sizeof buf, CASAN_HELLO, hid) ;
    option o (option::MO_Uri_Query, buf, strlen (buf)) ;
    pushoption (o) ;
}

void msg::mk_ctl_assoc (casantimer_t ttl, int mtu)
{
    char buf [MAXBUF] ;

    add_path_ctl () ;
    snprintf (buf, sizeof buf, CASAN_TTL, ttl) ;
    option o1 (option::MO_Uri_Query, buf, strlen (buf)) ;
    pushoption (o1) ;
    snprintf (buf, sizeof buf, CASAN_MTU, (long int) mtu) ;
    option o2 (option::MO_Uri_Query, buf, strlen (buf)) ;
    pushoption (o2) ;
}

}					// end of namespace casan
