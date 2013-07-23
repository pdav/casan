#ifndef SOS_L2802154_H
#define	SOS_L2802154_H

#include "l2.h"
#include <list>

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

	/*
	 * Buffer to accumulate bytes received from the XBee chip
	 */

	const int BUFLEN = 1024 ;
	byte buffer_ [MAXBUF] ;
	byte *pbuffer_ ;		// current item in buffer

	/*
	 * List of received frames
	 */

	enum frame_type {
	    TX_STATUS = 0x89,		// Status when TX request completed
	    RX_LONG = 0x80,		// Received packet with 64 bit addr
	    RX_SHORT = 0x81,		// Received packet with 16 bit addr
	} ;
	struct tx_status
	{
	    int frame_id ;
	    int status ;
	} ;
	const int RX_SHORT_OPT_BROADCAST = 0x02 ;
	struct rx_short
	{
	    int saddr ;			// source address
	    int len ;			// data length
	    byte data [L2802154MTU] ;	// data
	    int rssi ;
	    int options ;
	} ;
	struct frame
	{
	    frame_type type ;
	    union
	    {
		tx_status tx_status_ ;
		rx_short  rx_short_ ;
	    } ;
	} ;
	std::list <frame> framelist_ ;

	bool encode_transmit (byte *cmd, int &cmdlen, l2addr_802154 *daddr, byte *data, int len) ;
	int compute_checksum (const byte *buf, int paylen) ;
	bool valid_checksum(const byte*, int) ;
	int read_complete_frame (void) ;
	xbee_frame_status read_complete_frame (void) ;

	enum xbee_frame_status
	{
	    XBEE_INCOMPLETE,
	    XBEE_INVALID,
	    XBEE_VALID
	} ;
	xbee_frame_status frame_status (void) ;
} ;

}					// end of namespace sos
#endif
