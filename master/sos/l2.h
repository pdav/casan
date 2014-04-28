/**
 * @file l2.h
 * @brief l2addr and l2net virtual class interfaces
 */

#ifndef	SOS_L2_H
#define	SOS_L2_H

namespace sos {

/**
 * @brief return value for the l2net::recv method
 */

typedef enum pktype {
    PK_ME,		///< packet addressed to me
    PK_BCAST,		///< packet broadcasted
    PK_NONE		///< no packet (or not for me)
} pktype_t ;

/**
 * @brief Abstracts an address of any L2 network
 *
 * This class is a pure abstract class. It cannot be instancied.
 * Instead, the SOS engine keeps track of pointers to this class,
 * which are in reality pointers to objects of the `l2addr-xxx`
 * derived classes.
 */

class l2addr
{
    public:
	virtual ~l2addr () {} ;
	virtual bool operator== (const l2addr &other) = 0 ;
	virtual bool operator!= (const l2addr &other) = 0 ;

	// ugly hack to make operator<< feel as a virtual one
	friend std::ostream& operator<< (std::ostream &os, const l2addr &a) ;

    protected:
	virtual void print (std::ostream &os) const = 0 ;
} ;

/**
 * @brief Abstracts access to any L2 network
 *
 * This class is a pure abstract class. It cannot be instancied.
 * Instead, the SOS engine keeps track of the current network with
 * a pointer to this class, which is in reality a pointer to an
 * object of one of the `l2addr-xxx` derived classes, and it
 * performs all network accesses through this pointer.
 *
 * Each derived class provides some methods specific to the
 * L2 technology (specify Ethernet type for Ethernet, or channel
 * id for IEEE 802.15.4 for example), and a specific `init` method.
 */

class l2net
{
    public:
	virtual ~l2net () {} ;
	// no init method here: it is defined in each derived class
	virtual void term (void) = 0 ;
	virtual int send (l2addr *daddr, void *data, int len) = 0 ;
	virtual int bsend (void *data, int len) = 0 ;
	virtual pktype_t recv (l2addr **saddr, void *data, int *len) = 0 ;
	virtual l2addr * bcastaddr (void) = 0 ;

	int mtu (void) 		{ return mtu_ ; }
	int maxlatency (void) 	{ return maxlatency_ ; }

    protected:
	int mtu_ ;			// initialized in the init method
	int maxlatency_ ;		// initialized in the init method
} ;

}					// end of namespace sos
#endif
