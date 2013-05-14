#include <iostream>
#include <cstring>

#include "defs.h"
#include "sos.h"
#include "option.h"
#include "utils.h"

bool option::initialized_ = false ;
struct option::optdesc option::optdesc_ [256] ;


#define	OPTION_INIT		do { \
				    if (! initialized_) \
					static_init () ; \
				} while (false)			// no ";"
#define	RESET			do { \
				    optcode_ = MO_None ; \
				    optlen_ = 0 ; \
				    optval_ = 0 ; \
				} while (false)			// no ";"
#define	ALLOC_COPY(f,m,l)	do { \
				    f = new byte [(l)] ; \
				    std::memcpy (f, (m), (l)) ; \
				} while (false)			// no ";"
#define	COPY_VAL(p)		do { \
				    byte *b ; \
				    if (optlen_ > (int) sizeof staticval_) \
					b = optval_ = new byte [optlen_] ; \
				    else \
				    { \
					optval_ = 0 ; \
					b = staticval_ ; \
				    } \
				    std::memcpy (b, (p), optlen_) ; \
				} while (false)			// no ";"
#define	CHECK_OPTCODE(c)	do { \
				    if ((c) < 0 || (c) >= NTAB (optdesc_) || optdesc_ [c].format == OF_NONE) \
					throw 20 ; \
				} while (false)			// no ";"
#define	CHECK_OPTLEN(c,l)	do { \
				    if ((l) < optdesc_ [(c)].minlen \
					    || (l) > optdesc_ [(c)].maxlen) \
					throw 20 ; \
				} while (false)			// no ";"


/******************************************************************************
 * Utilities
 */

struct stbin
{
    byte bin [sizeof (option::uint)] ;
    int len ;
} ;

stbin *uint_to_byte (option::uint val)
{
    static stbin stbin ;
    int shft ;

    // translate in network byte order, without leading null bytes
    stbin.len = 0 ;
    for (shft = sizeof val - 1 ; shft >= 0 ; shft--)
    {
        byte b ;

        b = (val >> (shft * 8)) & 0xff ;
        if (stbin.len != 0 || b != 0)
            stbin.bin [stbin.len++] = b ;
    }
    return &stbin ;
}

/******************************************************************************
 * Constructors and destructors
 */

// default constructor
option::option ()
{
    OPTION_INIT ;
    RESET ;
}

option::option (optcode_t optcode)
{
    OPTION_INIT ;
    CHECK_OPTCODE (optcode) ;
    RESET ;
    optcode_ = optcode ;
}

option::option (optcode_t optcode, void *optval, int optlen)
{
    OPTION_INIT ;
    CHECK_OPTCODE (optcode) ;
    CHECK_OPTLEN (optcode, optlen) ;
    RESET ;
    optcode_ = optcode ;
    optlen_ = optlen ;
    COPY_VAL (optval) ;
}

option::option (optcode_t optcode, option::uint optval)
{
    stbin *stbin ;

    stbin = uint_to_byte (optval) ;
    OPTION_INIT ;
    CHECK_OPTCODE (optcode) ;
    CHECK_OPTLEN (optcode, stbin->len) ;
    RESET ;
    optcode_ = optcode ;
    optlen_ = stbin->len ;
    COPY_VAL (stbin->bin) ;
}

// copy constructor
option::option (const option &o)
{
    OPTION_INIT ;
    *this = o ;
    if (optval_)
	COPY_VAL (o.optval_) ;
}

// copy assignment constructor
option &option::operator= (const option &o)
{
    OPTION_INIT ;
    if (this != &o)
    {
	*this = o ;
	if (optval_)
	    COPY_VAL (o.optval_) ;
    }
    return *this ;
}

// default destructor
option::~option ()
{
    if (optval_)
	delete optval_ ;
}


void option::static_init (void)
{
    initialized_ = true ;

    for (int i = 0 ; i < NTAB (optdesc_) ; i++)
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
 * Accessors
 */

option::optcode_t option::optcode (void)
{
    return optcode_ ;
}

void *option::optval (int *len)
{
    *len = optlen_ ;
    return (optval_ == 0) ? staticval_ : optval_ ;
}

option::uint option::optval (void)
{
    option::uint v ;
    byte *b ;
    int i ;

    v = 0 ;
    b = (optval_ == 0) ? staticval_ : optval_ ;
    for (i = 0 ; i < optlen_ ; i++)
	v = (v << 8) & b [i] ;
    return v ;
}

/******************************************************************************
 * Mutators
 */

void option::optcode (optcode_t c)
{
    optcode_ = c;
}

void option::optval (void *val, int len)
{
    optlen_ = len ;
    COPY_VAL (val) ;
}

void option::optval (option::uint val)
{
    stbin *stbin ;

    stbin = uint_to_byte (val) ;
    CHECK_OPTLEN (optcode_, stbin->len) ;
    optlen_ = stbin->len ;
    COPY_VAL (stbin->bin) ;
}
