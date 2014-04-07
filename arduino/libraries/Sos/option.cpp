#include "defs.h"
#include "sos.h"
#include "option.h"

uint8_t option::errno_ = 0;

#define	RESET		do {					\
			    optcode_ = MO_None ;		\
			    optlen_ = 0 ;			\
			    optval_ = 0 ;			\
			} while (false)				// no ";"
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
			} while (false)				// no ";"
#define	CHK_OPTCODE(c,err) do {					\
			    int i ;				\
			    (err) = true ;			\
			    for (i = 0 ; i < MAXOPT ; i++)	\
			    {					\
				if (taboptdesc [i].code == c)	\
				{				\
				    (err) = false ;		\
				    break ;			\
				}				\
			    }					\
			} while (false)				// no ";"
#define	CHK_OPTLEN(c,l,err) do {				\
			    int i ;				\
			    (err) = true ;			\
			    for (i = 0 ; i < MAXOPT ; i++)	\
			    {					\
				if (taboptdesc [i].code == c	\
					&& (l) >= taboptdesc [i].minlen	\
					&& (l) <= taboptdesc [i].maxlen) \
				{				\
				    (err) = false ;		\
				    break ;			\
				}				\
			    }					\
			} while (false)				// no ";"


struct optdesc taboptdesc [] =
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

#define	MAXOPT		NTAB(taboptdesc)

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

void option::reset (void)
{
    if (optval_)
	free (optval_) ;
    RESET ;
}

// default constructor
option::option ()
{
    RESET ;
}

option::option (optcode_t optcode)
{
    bool err = false;
    CHK_OPTCODE (optcode, err) ;
    if (err)
    {
	PRINT_DEBUG_STATIC ("\033[31moption::optval err: CHK_OPTCODE 1\033[00m");
	option::errno_ = OPT_ERR_OPTCODE ;
    }
    RESET ;
    optcode_ = optcode ;
}

option::option (optcode_t optcode, void *optval, int optlen)
{
    bool err = false;
    CHK_OPTCODE (optcode, err) ;
    if (err)
    {
	PRINT_DEBUG_STATIC ("\033[31moption::optval err: CHK_OPTCODE 2\033[00m");
	option::errno_ = OPT_ERR_OPTCODE ;
    }
    CHK_OPTLEN (optcode, optlen, err) ;
    if (err)
    {
	PRINT_DEBUG_STATIC ("\033[31moption::optval err: CHK_OPTLEN 2\033[00m");
	option::errno_ = OPT_ERR_OPTLEN ;
    }
    RESET ;
    optcode_ = optcode ;
    optlen_ = optlen ;
    COPY_VAL (optval) ;
}

option::option (optcode_t optcode, option::uint optval)
{
    stbin *stbin ;

    stbin = uint_to_byte (optval) ;
    bool err = false;
    CHK_OPTCODE (optcode, err) ;
    if (err)
    {
	PRINT_DEBUG_STATIC ("\033[31moption::optval err: CHK_OPTCODE 3\033[00m");
	option::errno_ = OPT_ERR_OPTCODE;
    }
    CHK_OPTLEN (optcode, stbin->len, err) ;
    if (err)
    {
	PRINT_DEBUG_STATIC ("\033[31moption::optval err: CHK_OPTLEN 3\033[00m");
	option::errno_ = OPT_ERR_OPTLEN;
    }
    RESET ;
    optcode_ = optcode ;
    optlen_ = stbin->len ;
    COPY_VAL (stbin->bin) ;
}

// copy constructor
option::option (const option &o)
{
    memcpy (this, &o, sizeof o) ;
    if (optval_)
    {
	this->optval_ = NULL;
	COPY_VAL (o.optval_) ;
    }
}

// copy assignment constructor
option &option::operator= (const option &o)
{
    if (this != &o)
    {
	if (this->optval_)
	{
	    free (this->optval_);
	    this->optval_ = NULL;
	}
	memcpy (this, &o, sizeof *this) ;
	if (this->optval_)
	{
	    this->optval_ = NULL;
	    COPY_VAL (o.optval_) ;
	}
    }
    return *this ;
}

// default destructor
option::~option ()
{
    if (optval_)
	free (optval_) ;
}

/******************************************************************************
 * Operator used for list sorting (cf msg.cc)
 */

bool option::operator< (const option &o)
{
    return this->optcode_ < o.optcode_ ;
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

void * option::val (void)
{
    return (void *) (optval_ == 0) ? staticval_ : optval_ ;
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
    bool err = false;
    CHK_OPTLEN (optcode_, stbin->len, err) ;
    if (err)
    {
	PRINT_DEBUG_STATIC ("\033[31moption::optval err: CHK_OPTLEN\033[00m");
	option::errno_ = OPT_ERR_OPTLEN;
	return;
    }
    optlen_ = stbin->len ;
    COPY_VAL (stbin->bin) ;
}

int option::optlen (void)
{
    return optlen_;
}

uint8_t option::get_errno (void)
{
    return option::errno_;
}

void option::print (void)
{
    Serial.print (F ("\033[33mOPTION\033[00m : \033[36m optcode\033[00m="));
    switch ((unsigned char) optcode_)
    {
	case MO_None		: Serial.print (F ("MO_None")); break;
	case MO_Content_Format	: Serial.print (F ("MO_Content_Format")); break;
	case MO_Etag		: Serial.print (F ("MO_Etag")); break;
	case MO_Location_Path	: Serial.print (F ("MO_Location_Path")); break;
	case MO_Location_Query	: Serial.print (F ("MO_Location_Query")); break;
	case MO_Max_Age		: Serial.print (F ("MO_Max_Age")); break;
	case MO_Proxy_Uri	: Serial.print (F ("MO_Proxy_Uri")); break;
	case MO_Proxy_Scheme	: Serial.print (F ("MO_Proxy_Scheme")); break;
	case MO_Uri_Host	: Serial.print (F ("MO_Uri_Host")); break;
	case MO_Uri_Path	: Serial.print (F ("MO_Uri_Path")); break;
	case MO_Uri_Port	: Serial.print (F ("MO_Uri_Port")); break;
	case MO_Uri_Query	: Serial.print (F ("MO_Uri_Query")); break;
	case MO_Accept		: Serial.print (F ("MO_Accept")); break;
	case MO_If_None_Match	: Serial.print (F ("MO_If_None_Match")); break;
	case MO_If_Match	: Serial.print (F ("MO_If_Match")); break;
	default :
	    Serial.print (F ("\033[31mERROR\033[00m"));
	    Serial.print ((unsigned char)optcode_);
	    break;
    }
    Serial.print (F ("\033[36m optlen\033[00m="));
    Serial.print (optlen_);
    if (optval_)
    {
	Serial.print (F ("  \033[36moptval\033[00m="));
	char *buf = (char*) malloc (sizeof (char)*optlen_ +1);
	memcpy (buf, optval_, optlen_);
	buf[optlen_] = '\0';
	Serial.print (buf);
	free (buf);
    }
    else if (optlen_ > 0 )
    {
	Serial.print (F ("  \033[36mstaticval_\033[00m="));
	char *buf = (char*) malloc (sizeof (char)*optlen_ +1);
	memcpy (buf, staticval_, optlen_);
	buf[optlen_] = '\0';
	Serial.print (buf);
	free (buf);
    }
    Serial.println ();
}

void option::reset_errno (void)
{
    option::errno_ = 0;
}
