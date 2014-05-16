/**
 * @file option.cc
 * @brief option class implementation
 */

#include <iostream>
#include <cstring>
#include <vector>

#include "global.h"

#include "sos.h"
#include "option.h"
#include "utils.h"

#define	MAXOPT	256

namespace sos {
struct option::optdesc * option::optdesc_ = 0 ;


#define	OPTION_INIT	do {					\
			    if (optdesc_ == 0)			\
				static_init () ;		\
			} while (false)				// no ";"
#define	RESET		do {					\
			    optcode_ = MO_None ;		\
			    optlen_ = 0 ;			\
			    optval_ = 0 ;			\
			} while (false)				// no ";"
#define	DELETE_VAL(p)	do {					\
			    if (p)				\
			    {					\
				delete [] p ;			\
				p = 0 ;				\
			    }					\
			} while (false)				// no ";"
#define	COPY_VAL(p)	do {					\
			    byte *b ;				\
			    if (optlen_ + 1 > (int) sizeof staticval_) \
				b = optval_ = new byte [optlen_+ 1] ; \
			    else				\
			    {					\
				optval_ = 0 ;			\
				b = staticval_ ;		\
			    }					\
			    std::memcpy (b, (p), optlen_) ;	\
			    b [optlen_] = 0 ;			\
			} while (false)				// no ";"
#define	CHK_OPTCODE(c)	do {					\
			    if ((c) < 0 || (int) (c) >= MAXOPT	\
				|| optdesc_ [c].format == OF_NONE) \
				throw 20 ;			\
			} while (false)				// no ";"
#define	CHK_OPTLEN(c,l)	do {					\
			    if ((l) < optdesc_ [(c)].minlen	\
				    || (l) > optdesc_ [(c)].maxlen) \
				throw 20 ;			\
			} while (false)				// no ";"


/******************************************************************************
 * Utilities
 */

byte *option::uint_to_byte (uint val, int &len)
{
    static byte stbin [sizeof (uint)] ;
    int shft ;

    // translate in network byte order, without leading null bytes
    len = 0 ;
    for (shft = sizeof val - 1 ; shft >= 0 ; shft--)
    {
        byte b ;

        b = (val >> (shft * 8)) & 0xff ;
        if (len != 0 || b != 0)
            stbin [len++] = b ;
    }
    return stbin ;
}

/******************************************************************************
 * Constructors and destructors
 */

void option::reset (void)
{
    OPTION_INIT ;
    DELETE_VAL (optval_) ;
    RESET ;
}

/**
 * Default constructor
 *
 * The default constructor initializes option attributes
 */

option::option ()
{
    OPTION_INIT ;
    RESET ;
}

/**
 * Constructor for an empty option
 *
 * This constructor creates an option without any value, which can be
 * initialized later with the Option::optval method.
 *
 * @param optcode the option code
 */

option::option (optcode_t optcode)
{
    OPTION_INIT ;
    CHK_OPTCODE (optcode) ;
    RESET ;
    optcode_ = optcode ;
}

/**
 * Constructor for an option with an opaque value
 *
 * This constructor creates an option with an opaque format value.
 * The value itself is copied in the option.
 *
 * @param optcode the option code
 * @param optval pointer to value
 * @param optlen length of value
 */

option::option (optcode_t optcode, const void *optval, int optlen)
{
    OPTION_INIT ;
    CHK_OPTCODE (optcode) ;
    CHK_OPTLEN (optcode, optlen) ;
    RESET ;
    optcode_ = optcode ;
    optlen_ = optlen ;
    COPY_VAL (optval) ;
}

/**
 * Constructor for an option with an integer value
 *
 * This constructor creates an option with an integer value. This
 * value will be converted (packed) in the minimal string of bytes
 * according to the CoAP specification.
 *
 * @param optcode the option code
 * @param optval integer value
 */

option::option (optcode_t optcode, option::uint optval)
{
    byte *stbin ;
    int len ;

    stbin = uint_to_byte (optval, len) ;
    OPTION_INIT ;
    CHK_OPTCODE (optcode) ;
    CHK_OPTLEN (optcode, len) ;
    RESET ;
    optcode_ = optcode ;
    optlen_ = len ;
    COPY_VAL (stbin) ;
}

/**
 * Copy constructor
 *
 * This constructor copies an existing option
 */

option::option (const option &o)
{
    OPTION_INIT ;
    std::memcpy (this, &o, sizeof o) ;
    if (optval_)
	COPY_VAL (o.optval_) ;
}

/**
 * Copy assignment constructor
 *
 * This constructor copies an existing option
 */

option &option::operator= (const option &o)
{
    OPTION_INIT ;
    if (this != &o)
    {
	DELETE_VAL (optval_) ;
	std::memcpy (this, &o, sizeof o) ;
	if (optval_)
	    COPY_VAL (o.optval_) ;
    }
    return *this ;
}

/**
 * Default destructor
 */

option::~option ()
{
    DELETE_VAL (optval_) ;
}


void option::static_init (void)
{
    optdesc_ = new struct optdesc [MAXOPT] ;

    for (int i = 0 ; i < MAXOPT ; i++)
	optdesc_ [i].format = OF_NONE ;

    optdesc_ [MO_Content_Format].format = OF_OPAQUE ;
    optdesc_ [MO_Content_Format].minlen = 0 ;
    optdesc_ [MO_Content_Format].maxlen = 8 ;

    optdesc_ [MO_Etag].format = OF_OPAQUE ;
    optdesc_ [MO_Etag].minlen = 1 ;
    optdesc_ [MO_Etag].maxlen = 8 ;

    optdesc_ [MO_Location_Path].format = OF_STRING ;
    optdesc_ [MO_Location_Path].minlen = 0 ;
    optdesc_ [MO_Location_Path].maxlen = 255 ;

    optdesc_ [MO_Location_Query].format = OF_STRING ;
    optdesc_ [MO_Location_Query].minlen = 0 ;
    optdesc_ [MO_Location_Query].maxlen = 255 ;

    optdesc_ [MO_Max_Age].format = OF_UINT ;
    optdesc_ [MO_Max_Age].minlen = 0 ;
    optdesc_ [MO_Max_Age].maxlen = 4 ;

    optdesc_ [MO_Proxy_Uri].format = OF_STRING ;
    optdesc_ [MO_Proxy_Uri].minlen = 1 ;
    optdesc_ [MO_Proxy_Uri].maxlen = 1034 ;

    optdesc_ [MO_Proxy_Scheme].format = OF_STRING ;
    optdesc_ [MO_Proxy_Scheme].minlen = 1 ;
    optdesc_ [MO_Proxy_Scheme].maxlen = 255 ;

    optdesc_ [MO_Uri_Host].format = OF_STRING ;
    optdesc_ [MO_Uri_Host].minlen = 1 ;
    optdesc_ [MO_Uri_Host].maxlen = 255 ;

    optdesc_ [MO_Uri_Path].format = OF_STRING ;
    optdesc_ [MO_Uri_Path].minlen = 0 ;
    optdesc_ [MO_Uri_Path].maxlen = 255 ;

    optdesc_ [MO_Uri_Port].format = OF_UINT ;
    optdesc_ [MO_Uri_Port].minlen = 0 ;
    optdesc_ [MO_Uri_Port].maxlen = 2 ;

    optdesc_ [MO_Uri_Query].format = OF_STRING ;
    optdesc_ [MO_Uri_Query].minlen = 0 ;
    optdesc_ [MO_Uri_Query].maxlen = 255 ;

    optdesc_ [MO_Accept].format = OF_UINT ;
    optdesc_ [MO_Accept].minlen = 0 ;
    optdesc_ [MO_Accept].maxlen = 2 ;

    optdesc_ [MO_If_None_Match].format = OF_EMPTY ;
    optdesc_ [MO_If_None_Match].minlen = 0 ;
    optdesc_ [MO_If_None_Match].maxlen = 0 ;

    optdesc_ [MO_If_Match].format = OF_OPAQUE ;
    optdesc_ [MO_If_Match].minlen = 0 ;
    optdesc_ [MO_If_Match].maxlen = 8 ;
}

/******************************************************************************
 * Operators
 */

/** @brief Mainly used for debugging purpose
 */

std::ostream& operator<< (std::ostream &os, const option &o)
{
    os << "option <code=" << o.optcode_ << ", len=" << o.optlen_ << ">" ;
    return os ;
}

/** @brief Operator used for list sorting (cf msg.cc)
 */

bool option::operator< (const option &o) const
{
    return this->optcode_ < o.optcode_ ;
}

/******************************************************************************
 * Operator used for option matching
 */

bool option::operator== (const option &o)
{
    bool r = false ;

    if (optcode_ == o.optcode_ && optlen_ == o.optlen_)
    {
	if (optval_ && o.optval_)
	    r = (std::memcmp (optval_, o.optval_, optlen_) == 0) ;
	else r = (std::memcmp (staticval_, o.staticval_, optlen_) == 0) ;
    }
    return r ;
}

bool option::operator!= (const option &o)
{
    return ! (*this == o) ;
}

/******************************************************************************
 * Accessors
 */

/**
 * Get the option code
 */

option::optcode_t option::optcode (void)
{
    return optcode_ ;
}

/**
 * Get the option value and length
 *
 * @param len address of an integer which will contain the length
 *	in return, or null address if length is not wanted
 * @return pointer to the option value (do not free this address)
 */

void *option::optval (int *len)
{
    *len = optlen_ ;
    return (optval_ == 0) ? staticval_ : optval_ ;
}

/**
 * Get the option value as an integer
 *
 * The option value is unpacked as an integer.
 *
 * @return option value as a standard (unsigned) integer
 */

option::uint option::optval (void)
{
    option::uint v ;
    byte *b ;
    int i ;

    v = 0 ;
    b = (optval_ == 0) ? staticval_ : optval_ ;
    for (i = 0 ; i < optlen_ ; i++)
	v = (v << 8) | b [i] ;
    return v ;
}

/******************************************************************************
 * Mutators
 */

/**
 * Assign the option code
 */

void option::optcode (optcode_t c)
{
    optcode_ = c;
}

/**
 * Assign an opaque value to the option
 *
 * @param val pointer to the value to be copied in the option
 * @param len length of value
 */

void option::optval (void *val, int len)
{
    DELETE_VAL (optval_) ;
    optlen_ = len ;
    COPY_VAL (val) ;
}

/**
 * Assign an integer value to the option
 *
 * The value will be converted (packed) in the minimal string of bytes
 * according to the CoAP specification.
 *
 * @param val integer to copy in option
 */

void option::optval (option::uint val)
{
    byte *stbin ;
    int len ;

    DELETE_VAL (optval_) ;
    stbin = uint_to_byte (val, len) ;
    CHK_OPTLEN (optcode_, len) ;
    optlen_ = len ;
    COPY_VAL (stbin) ;
}

}					// end of namespace sos
