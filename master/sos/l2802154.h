#ifndef SOS_L2802154_H
#define	SOS_L2802154_H

#include "l2.h"

// Ethernet address length
#define	L2802154ADDRLEN	8
// Ethernet default MTU
#define	L2802154MTU	127
// Ethernet maximum latency (ms). Even a very conservative value is far
// more realistic than the default CoAP value (100 s)
// #define	ETHMAXLATENCY	10
#define	L2802154MAXLATENCY	50


namespace sos {

class l2addr_802154: public l2addr
{
    public:
	l2addr_802154 () ;			// default constructor
	l2addr_802154 (const char *) ;		// constructor
	l2addr_802154 (const l2addr_802154 &l) ; // copy constructor
	l2addr_802154 &operator= (const l2addr_802154 &l) ;	// copy assignment constructor

	bool operator== (const l2addr &) ;
	bool operator!= (const l2addr &) ;

    protected:
	void print (std::ostream &os) const ;
	byte addr_ [L2802154ADDRLEN] ;

	friend class l2net_802154 ;
} ;

extern l2addr_802154 l2addr_802154_broadcast ;

class l2net_802154: public l2net
{
    public:
	~l2net_802154 () {} ;
	// type = xbee
	int init (const std::string iface, const char *type, const std::string myaddr, const std::string panid) ;
	void term (void) ;
	int send (l2addr *daddr, void *data, int len) ;
	int bsend (void *data, int len) ;
	pktype_t recv (l2addr **saddr, void *data, int *len) ;
	l2addr *bcastaddr (void) ;

    private:
	int fd_ ;			// interface index
	int panid_ ;			// PAN id

	bool encode_transmit (byte *cmd, int &cmdlen, l2addr_802154 *daddr, byte *data, int len) ;
	int compute_checksum (const byte *buf, int paylen) ;
} ;

}					// end of namespace sos
#endif
