/**
 * @file option.cpp
 * @brief option class implementation
 */

#include "option.h"

uint8_t option::errno_ = 0 ;

#define	RESET		do {					\
			    optcode_ = MO_None ;		\
			    optlen_ = 0 ;			\
			    optval_ = 0 ;			\
			} while (false)				// no " ;"
#define	COPY_VAL(p)	do {					\
			    byte *b ;				\
			    if (optlen_ + 1 > (int) sizeof staticval_) { \
				optval_ = (uint8_t*) malloc (optlen_+ 1) ; \
				b = optval_ ;			\
			    }					\
			    else				\
			    {					\
				optval_ = 0 ;			\
				b = staticval_ ;		\
			    }					\
			    memcpy (b, (p), optlen_) ;		\
			    b [optlen_] = 0 ;			\
			} while (false)				// no " ;"
#define	CHK_OPTCODE(c,err) do {					\
			    int i ;				\
			    (err) = true ;			\
			    for (i = 0 ; i < NTAB (optdesc_) ; i++)	\
			    {					\
				if (optdesc_ [i].code == c)	\
				{				\
				    (err) = false ;		\
				    break ;			\
				}				\
			    }					\
			} while (false)				// no " ;"
#define	CHK_OPTLEN(c,l,err) do {				\
			    int i ;				\
			    (err) = true ;			\
			    for (i = 0 ; i < NTAB (optdesc_) ; i++)	\
			    {					\
				if (optdesc_ [i].code == c	\
					&& (l) >= optdesc_ [i].minlen	\
					&& (l) <= optdesc_ [i].maxlen) \
				{				\
				    (err) = false ;		\
				    break ;			\
				}				\
			    }					\
			} while (false)				// no " ;"


option::optdesc option::optdesc_ [] =
{
    { option::MO_Content_Format,	option::OF_OPAQUE,	0, 8	},
    { option::MO_Etag,			option::OF_OPAQUE,	1, 8	},
    { option::MO_Location_Path,		option::OF_STRING,	0, 255	},
    { option::MO_Location_Query,	option::OF_STRING,	0, 255	},
    { option::MO_Max_Age,		option::OF_UINT,	0, 4	},
    { option::MO_Proxy_Uri,		option::OF_STRING,	1, 1034	},
    { option::MO_Proxy_Scheme,		option::OF_STRING,	1, 255	},
    { option::MO_Uri_Host,		option::OF_STRING,	1, 255	},
    { option::MO_Uri_Path,		option::OF_STRING,	0, 255	},
    { option::MO_Uri_Port,		option::OF_UINT,	0, 2	},
    { option::MO_Uri_Query,		option::OF_STRING,	0, 255	},
    { option::MO_Accept,		option::OF_UINT,	0, 2	},
    { option::MO_If_None_Match,		option::OF_EMPTY,	0, 0	},
    { option::MO_If_Match,		option::OF_OPAQUE,	0, 8	},
} ;

/******************************************************************************
 * Utilities
 */

option::byte *option::uint_to_byte (uint val, int &len)
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
    if (optval_)
	free (optval_) ;
    RESET ;
}

/**
 * Default constructor
 *
 * The default constructor initializes option attributes
 */

option::option ()
{
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
    bool err = false ;
    CHK_OPTCODE (optcode, err) ;
    if (err)
    {
	Serial.println (F (RED ("option::optval err: CHK_OPTCODE 1"))) ;
	option::errno_ = OPT_ERR_OPTCODE ;
    }
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
    bool err = false ;
    CHK_OPTCODE (optcode, err) ;
    if (err)
    {
	Serial.println (F (RED ("option::optval err: CHK_OPTCODE 2"))) ;
	option::errno_ = OPT_ERR_OPTCODE ;
    }
    CHK_OPTLEN (optcode, optlen, err) ;
    if (err)
    {
	Serial.println (F (RED ("option::optval err: CHK_OPTLEN 2"))) ;
	option::errno_ = OPT_ERR_OPTLEN ;
    }
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
    bool err ;
    byte *stbin ;
    int len ;

    stbin = uint_to_byte (optval, len) ;
    err = false ;
    CHK_OPTCODE (optcode, err) ;
    if (err)
    {
	Serial.println (F (RED ("option::optval err: CHK_OPTCODE 3"))) ;
	option::errno_ = OPT_ERR_OPTCODE ;
    }
    CHK_OPTLEN (optcode, len, err) ;
    if (err)
    {
	Serial.println (F (RED ("option::optval err: CHK_OPTLEN 3"))) ;
	option::errno_ = OPT_ERR_OPTLEN ;
    }
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
    memcpy (this, &o, sizeof o) ;
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
    if (this != &o)
    {
	if (this->optval_)
	{
	    free (this->optval_) ;
	    this->optval_ = NULL ;
	}
	memcpy (this, &o, sizeof *this) ;
	if (o.optval_)
	    COPY_VAL (o.optval_) ;
    }
    return *this ;
}

/**
 * Default destructor
 */

option::~option ()
{
    if (optval_)
	free (optval_) ;
}

/******************************************************************************
 * Operator used for list sorting (cf msg.cc)
 */

bool option::operator== (const option &o)
{
    return this->optcode_ == o.optcode_ ;
}
bool option::operator!= (const option &o)
{
    return this->optcode_ != o.optcode_ ;
}

bool option::operator<  (const option &o)
{
    return this->optcode_ < o.optcode_ ;
}

bool option::operator<= (const option &o)
{
    return this->optcode_ <= o.optcode_ ;
}

bool option::operator>  (const option &o)
{
    return this->optcode_ > o.optcode_ ;
}

bool option::operator>= (const option &o)
{
    return this->optcode_ >= o.optcode_ ;
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
    if (len != (int *) 0)
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
	v = (v << 8) & b [i] ;
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
    optcode_ = c ;
}

/**
 * Assign an opaque value to the option
 *
 * @param val pointer to the value to be copied in the option
 * @param len length of value
 */

void option::optval (void *val, int len)
{
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
    bool err ;
    byte *stbin ;
    int len ;

    stbin = uint_to_byte (val, len) ;
    err = false ;
    CHK_OPTLEN (optcode_, len, err) ;
    if (err)
    {
	Serial.println (F (RED ("option::optval err: CHK_OPTLEN"))) ;
	option::errno_ = OPT_ERR_OPTLEN ;
	return ;
    }
    optlen_ = len ;
    COPY_VAL (stbin) ;
}

/**
 * Option length
 *
 * The option length does not include the added null byte used internally.
 *
 * @return length of option value
 */

int option::optlen (void)
{
    return optlen_ ;
}

/**
 * Returns the last error encountered during an option assignment
 */

uint8_t option::get_errno (void)
{
    return option::errno_ ;
}

void option::print (void)
{
    Serial.print (F (YELLOW ("OPTION") " : " RED ("optcode") "=")) ;
    switch ((unsigned char) optcode_)
    {
	case MO_None		: Serial.print (F("MO_None")) ; break ;
	case MO_Content_Format	: Serial.print (F("MO_Content_Format")) ; break;
	case MO_Etag		: Serial.print (F("MO_Etag")) ; break ;
	case MO_Location_Path	: Serial.print (F("MO_Location_Path")) ; break ;
	case MO_Location_Query	: Serial.print (F("MO_Location_Query")) ; break;
	case MO_Max_Age		: Serial.print (F("MO_Max_Age")) ; break ;
	case MO_Proxy_Uri	: Serial.print (F("MO_Proxy_Uri")) ; break ;
	case MO_Proxy_Scheme	: Serial.print (F("MO_Proxy_Scheme")) ; break ;
	case MO_Uri_Host	: Serial.print (F("MO_Uri_Host")) ; break ;
	case MO_Uri_Path	: Serial.print (F("MO_Uri_Path")) ; break ;
	case MO_Uri_Port	: Serial.print (F("MO_Uri_Port")) ; break ;
	case MO_Uri_Query	: Serial.print (F("MO_Uri_Query")) ; break ;
	case MO_Accept		: Serial.print (F("MO_Accept")) ; break ;
	case MO_If_None_Match	: Serial.print (F("MO_If_None_Match")) ; break ;
	case MO_If_Match	: Serial.print (F("MO_If_Match")) ; break ;
	default :
	    Serial.print (F (RED ("ERROR"))) ;
	    Serial.print ((unsigned char) optcode_) ;
	    break ;
    }
    Serial.print (F ("/")) ;
    Serial.print (optcode_) ;
    Serial.print (F (BLUE (" optlen") "=")) ;
    Serial.print (optlen_) ;

    if (optval_)
    {
	Serial.print (F (BLUE (" optval") "=")) ;
	char *buf = (char *) malloc (sizeof (char) * optlen_ + 1) ;
	memcpy (buf, optval_, optlen_) ;
	buf[optlen_] = '\0' ;
	Serial.print (buf) ;
	free (buf) ;
    }
    else if (optlen_ > 0 )
    {
	Serial.print (F (BLUE (" staticval") "=")) ;
	char *buf = (char *) malloc (sizeof (char) * optlen_ + 1) ;
	memcpy (buf, staticval_, optlen_) ;
	buf[optlen_] = '\0' ;
	Serial.print (buf) ;
	free (buf) ;
    }
    Serial.println () ;
}

void option::reset_errno (void)
{
    option::errno_ = 0 ;
}
