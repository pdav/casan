/**
 * @file option.h
 * @brief option class interface
 */

#ifndef CASAN_OPTION_H
#define CASAN_OPTION_H

namespace casan {

/**
 * @brief An object of class option represents an individual option
 *
 * This class represents options associated with a message. options are
 * specified by the CoAP protocol.
 *
 * Before sending a message, application must associate options to this
 * message (with the Msg::push_option method).
 * They will be encoded, according to CoAP specification, when
 * the message is about to be sent.
 *
 * Upon message reception, the message will be decoded: options will
 * be created and associated with the received message. Application may
 * retrieve them (with the Msg::reset_next_option and Msg::next_option
 * methods).
 *
 * When an option is created, some points (format, minimum and maximum
 * length) will be checked according to a private table.
 * The format of an option may be:
 * * a string
 * * an unsigned integer
 * * an opaque value
 * * nothing
 *
 * An option opaque value is always internally represented as a byte
 * string, with an added '\0' at the end. This null byte will not be
 * sent on the network, of course, but allows for string manipulation
 * (with functions such as `strlen` or `strcmp` for example) on values
 * returned by option::optval method.
 * Integer options, on another hand, are internally represented as the
 * minimal byte string, according to the CoAP specification.
 * Thus, the value 255 is represented as one byte, whereas 65537 is
 * represented as 3 bytes. This should be completely transparent to
 * applications.
 */

class option
{
    public:
	typedef enum {
			MO_None			= 0,
			MO_Content_Format	= 12,
			MO_Etag			= 4,
			MO_Location_Path	= 8,
			MO_Location_Query	= 20,
			MO_Max_Age		= 14,
			MO_Proxy_Uri		= 35,
			MO_Proxy_Scheme		= 39,
			MO_Uri_Host		= 3,
			MO_Uri_Path		= 11,
			MO_Uri_Port		= 7,
			MO_Uri_Query		= 15,
			MO_Accept		= 16,
			MO_If_None_Match	= 5,
			MO_If_Match		= 1,
			MO_Size1		= 60,
		    } optcode_t ;
	typedef unsigned long int uint ;

	option () ;				// constructor
	option (optcode_t c) ;			// constructor
	option (optcode_t c, const void *v, int l) ; // constructor
	option (optcode_t c, uint v) ;		// constructor
	option (const option &o) ;		// copy constructor
	option &operator= (const option &o) ;	// copy assignment constructor
	~option () ;				// destructor

	friend std::ostream& operator<< (std::ostream &os, const option &o) ;

	bool operator< (const option &o) const ; // for list sorting in msg.cc

	bool operator== (const option &o) ;
	bool operator!= (const option &o) ;

	void reset (void) ;

	// accessors
	optcode_t optcode (void) ;
	void *optval (int *len) ;
	uint optval (void) ;

	// see Coap spec (5.4.6)
	bool critical (void)   { return (int) optcode_ & 1 ; }
	bool unsafe (void)     { return (int) optcode_ & 2 ; }
	bool nocachekey (void) { return ((int) optcode_ & 0x1e) == 0x1c ; }

	// mutators
	void optcode (optcode_t c) ;
	void optval (void *v, int len) ;
	void optval (uint v) ;

    protected:
	optcode_t optcode_ = MO_None ;
	int optlen_ = 0 ;
	byte *optval_ = 0 ;		// 0 if staticval is used
	byte staticval_ [8 + 1] ;	// keep a \0 after, just in case

	friend class msg ;

    private:
	typedef enum optfmt { OF_NONE = 0, OF_OPAQUE, OF_STRING,
	    				OF_EMPTY, OF_UINT } optfmt_t ;
	struct optdesc
	{
	    optfmt_t format ;
	    int minlen ;
	    int maxlen ;
	} ;
	static optdesc *optdesc_ ;

	static byte *uint_to_byte (uint val, int &len) ;

	void static_init (void) ;
} ;

}					// end of namespace casan
#endif
