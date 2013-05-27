#include "defs.h"
#include "sos.h"
#include "option.h"

struct option::optdesc * option::optdesc_ = 0 ;
uint8_t option::errno_ = 0;

#define	OPTION_INIT	do {					\
	if (optdesc_ == 0)			\
	static_init () ;		\
} while (false)				// no ";"
#define	RESET		do {					\
	optcode_ = MO_None ;		\
	optlen_ = 0 ;			\
	optval_ = 0 ;			\
} while (false)				// no ";"
#define	COPY_VAL(p)	do {							\
	byte *b ;										\
	if (optlen_ + 1> (int) sizeof staticval_) {		\
		optval_ = (uint8_t*) malloc(optlen_+ 1) ;	\
		b = optval_;								\
	}												\
	else											\
	{												\
		optval_ = 0 ;								\
		b = staticval_ ;							\
	}												\
	memcpy (b, (p), optlen_) ;						\
	b [optlen_] = 0 ;								\
} while (false)				// no ";"
#define	CHK_OPTCODE(c,err)	do {					\
	if ((c) < 0 || (c) >= MAXOPT)					\
		err = true;									\
	for(int i(0) ; !err && i < MAXOPT ; i++)		\
		if(optdesc_[i].code == c &&					\
				optdesc_ [i].format == OF_NONE)		\
			err = true;\
} while (false)				// no ";"
#define	CHK_OPTLEN(c,l,err)	do {					\
	for(int i(0) ; !err && i < MAXOPT ; i++) {		\
		if (optdesc_[i].code == c && (				\
				(l) < optdesc_ [i].minlen			\
				|| (l) > optdesc_ [c].maxlen))		\
		err = true;									\
	}												\
} while (false)				// no ";"


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
	OPTION_INIT ;
	if (optval_)
		free(optval_) ;
	RESET ;
}

// default constructor
option::option ()
{
	OPTION_INIT ;
	RESET ;
}

option::option (optcode_t optcode)
{
	OPTION_INIT ;
	bool err = false;
	CHK_OPTCODE (optcode, err) ;
	if (err) {
		PRINT_DEBUG_STATIC("\033[31moption::optval err : CHK_OPTCODE 1\033[00m");
		option::errno_ = OPT_ERR_OPTCODE;
	}
	RESET ;
	optcode_ = optcode ;
}

option::option (optcode_t optcode, void *optval, int optlen)
{
	OPTION_INIT ;
	bool err = false;
	CHK_OPTCODE (optcode, err) ;
	if(err) {
		PRINT_DEBUG_STATIC("\033[31moption::optval err : CHK_OPTCODE 2\033[00m");
		option::errno_ = OPT_ERR_OPTCODE;
	}
	CHK_OPTLEN (optcode, optlen, err) ;
	if(err) {
		PRINT_DEBUG_STATIC("\033[31moption::optval err : CHK_OPTLEN 2\033[00m");
		option::errno_ = OPT_ERR_OPTLEN;
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
	OPTION_INIT ;
	bool err = false;
	CHK_OPTCODE (optcode, err) ;
	if(err) {
		PRINT_DEBUG_STATIC("\033[31moption::optval err : CHK_OPTCODE 3\033[00m");
		option::errno_ = OPT_ERR_OPTCODE;
	}
	CHK_OPTLEN (optcode, stbin->len, err) ;
	if(err) {
		PRINT_DEBUG_STATIC("\033[31moption::optval err : CHK_OPTLEN 3\033[00m");
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
	OPTION_INIT ;
	memcpy (this, &o, sizeof o) ;
	if (optval_) {
		this->optval_ = NULL;
		COPY_VAL (o.optval_) ;
	}
}

// copy assignment constructor
option &option::operator= (const option &o)
{
	OPTION_INIT ;
	if (this != &o)
	{
		if (this->optval_) {
			free(this->optval_);
			this->optval_ = NULL;
		}
		memcpy (this, &o, sizeof *this) ;
		if (this->optval_) {
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
		free(optval_) ;
}


void option::static_init (void)
{
	optdesc_ = (struct optdesc*) malloc(sizeof(struct optdesc) *MAXOPT) ;

	for (int i = 0 ; i < MAXOPT ; i++)
		optdesc_ [i].format = OF_NONE ;

	optdesc_ [0].code = MO_Content_Format;
	optdesc_ [0].format = OF_OPAQUE ;
	optdesc_ [0].minlen = 0 ;
	optdesc_ [0].maxlen = 8 ;

	optdesc_ [1].code = MO_Etag;
	optdesc_ [1].format = OF_OPAQUE ;
	optdesc_ [1].minlen = 1 ;
	optdesc_ [1].maxlen = 8 ;

	optdesc_ [2].code = MO_Location_Path;
	optdesc_ [2].format = OF_STRING ;
	optdesc_ [2].minlen = 0 ;
	optdesc_ [2].maxlen = 255 ;

	optdesc_ [3].code = MO_Location_Query;
	optdesc_ [3].format = OF_STRING ;
	optdesc_ [3].minlen = 0 ;
	optdesc_ [3].maxlen = 255 ;

	optdesc_ [4].code = MO_Max_Age;
	optdesc_ [4].format = OF_UINT ;
	optdesc_ [4].minlen = 0 ;
	optdesc_ [4].maxlen = 4 ;

	optdesc_ [5].code = MO_Proxy_Uri;
	optdesc_ [5].format = OF_STRING ;
	optdesc_ [5].minlen = 1 ;
	optdesc_ [5].maxlen = 1034 ;

	optdesc_ [6].code = MO_Proxy_Scheme;
	optdesc_ [6].format = OF_STRING ;
	optdesc_ [6].minlen = 1 ;
	optdesc_ [6].maxlen = 255 ;

	optdesc_ [7].code = MO_Uri_Host;
	optdesc_ [7].format = OF_STRING ;
	optdesc_ [7].minlen = 1 ;
	optdesc_ [7].maxlen = 255 ;

	optdesc_ [8].code = MO_Uri_Path;
	optdesc_ [8].format = OF_STRING ;
	optdesc_ [8].minlen = 0 ;
	optdesc_ [8].maxlen = 255 ;

	optdesc_ [9].code = MO_Uri_Port;
	optdesc_ [9].format = OF_UINT ;
	optdesc_ [9].minlen = 0 ;
	optdesc_ [9].maxlen = 2 ;

	optdesc_ [10].code = MO_Uri_Query;
	optdesc_ [10].format = OF_STRING ;
	optdesc_ [10].minlen = 0 ;
	optdesc_ [10].maxlen = 255 ;

	optdesc_ [11].code = MO_Accept;
	optdesc_ [11].format = OF_UINT ;
	optdesc_ [11].minlen = 0 ;
	optdesc_ [11].maxlen = 2 ;

	optdesc_ [12].code = MO_If_None_Match;
	optdesc_ [12].format = OF_EMPTY ;
	optdesc_ [12].minlen = 0 ;
	optdesc_ [12].maxlen = 0 ;

	optdesc_ [13].code = MO_If_Match;
	optdesc_ [13].format = OF_OPAQUE ;
	optdesc_ [13].minlen = 0 ;
	optdesc_ [13].maxlen = 8 ;
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
	if(err) {
		PRINT_DEBUG_STATIC("\033[31moption::optval err : CHK_OPTLEN\033[00m");
		option::errno_ = OPT_ERR_OPTLEN;
		return;
	}
	optlen_ = stbin->len ;
	COPY_VAL (stbin->bin) ;
}

uint8_t option::get_errno(void) {
	return option::errno_;
}

void option::print(void) {
	Serial.print(F("\033[33mOPTION\033[00m : \033[36m optcode\033[00m="));
	switch(optcode_) {
		case MO_None			: Serial.print(F("MO_None")); break;
		case MO_Content_Format	: Serial.print(F("MO_Content_Format")); break;
		case MO_Etag			: Serial.print(F("MO_Etag")); break;
		case MO_Location_Path	: Serial.print(F("MO_Location_Path")); break;
		case MO_Location_Query	: Serial.print(F("MO_Location_Query")); break;
		case MO_Max_Age			: Serial.print(F("MO_Max_Age")); break;
		case MO_Proxy_Uri		: Serial.print(F("MO_Proxy_Uri")); break;
		case MO_Proxy_Scheme	: Serial.print(F("MO_Proxy_Scheme")); break;
		case MO_Uri_Host		: Serial.print(F("MO_Uri_Host")); break;
		case MO_Uri_Path		: Serial.print(F("MO_Uri_Path")); break;
		case MO_Uri_Port		: Serial.print(F("MO_Uri_Port")); break;
		case MO_Uri_Query		: Serial.print(F("MO_Uri_Query")); break;
		case MO_Accept			: Serial.print(F("MO_Accept")); break;
		case MO_If_None_Match	: Serial.print(F("MO_If_None_Match")); break;
		case MO_If_Match		: Serial.print(F("MO_If_Match")); break;
		default :
			Serial.print(F("\033[31mERROR\033[00m"));
			Serial.print(optcode_);
			break;
	}
	Serial.print(F("\033[36m optlen\033[00m="));
	Serial.print(optlen_);
	if(optval_) {
		Serial.print(F("  \033[36moptval\033[00m="));
		char *buf = (char*) malloc(sizeof(char)*optlen_ +1);
		memcpy(buf, optval_, optlen_);
		buf[optlen_] = '\0';
		Serial.print(buf);
		free(buf);
	}
	else if (optlen_ > 0 ) {
		Serial.print(F("  \033[36mstaticval_\033[00m="));
		char *buf = (char*) malloc(sizeof(char)*optlen_ +1);
		memcpy(buf, staticval_, optlen_);
		buf[optlen_] = '\0';
		Serial.print(buf);
		free(buf);
	}
	Serial.println();
}

void option::reset_errno(void) {
	option::errno_ = 0;
}
