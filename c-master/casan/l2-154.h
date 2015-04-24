/**
 * @file l2-154.h
 * @brief l2addr and l2net specializations for IEEE 802.15.4
 */

#ifndef CASAN_L2154_H
#define	CASAN_L2154_H

#include "l2.h"
#include <list>

// IEEE 802.15.4 address length
#define	L2154ADDRLEN	8
// IEEE 802.15.4 default MTU
#define	L2154MTU	127
// IEEE 802.15.4 maximum latency (ms). Even a very conservative value is far
// more realistic than the default CoAP value (100 s)
// In our case, 128 Bytes takes ~4 ms with a 230 kb/s network
#define	L2154MAXLATENCY	5


namespace casan {

/**
 * @brief Specialization of the l2addr abstract class for IEEE 802.15.4
 *
 * This class provides real methods (specified in l2addr virtual
 * class) for IEEE 802.15.4 addresses.
 *
 * @bug 16-bits addresses are supported as 64-bits filled with 0s
 */

class l2addr_154: public l2addr
{
    public:
	l2addr_154 () ;			// default constructor
	l2addr_154 (const char *) ;		// constructor
	l2addr_154 (const l2addr_154 &l) ; // copy constructor
	l2addr_154 &operator= (const l2addr_154 &l) ;	// copy assignment constructor

	bool operator== (const l2addr &) ;
	bool operator!= (const l2addr &) ;

    protected:
	void print (std::ostream &os) const ;
	byte addr_ [L2154ADDRLEN] ;

	friend class l2net_154 ;
} ;

extern l2addr_154 l2addr_154_broadcast ;

/**
 * @brief Specialization of the l2net abstract class for IEEE 802.15.4
 *
 * This class provides real methods (specified in l2net virtual
 * class) for IEEE 802.15.4 networks. It handles the lowest layer,
 * which is assumed to be the XBee USB stick.
 */

class l2net_154: public l2net
{
    public:
	~l2net_154 () {} ;
	// type = xbee
	int init (const std::string iface, const char *type, const int mtu, const std::string myaddr, const std::string panid, const int channel) ;
	void term (void) ;
	int send (l2addr *daddr, void *data, int len) ;
	int bsend (void *data, int len) ;
	pktype_t recv (l2addr **saddr, void *data, int *len) ;
	l2addr *bcastaddr (void) ;

    private:
	int fd_ ;			// interface index

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
	    byte data [L2154MTU] ;	// data
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

	bool encode_transmit (byte *cmd, int &cmdlen, l2addr_154 *daddr, byte *data, int len) ;
	int compute_checksum (const byte *buf) ;
	pktype_t extract_received_packet (l2addr **saddr, void *data, int *len) ;
	int read_complete_frame (void) ;
	bool is_frame_complete (void) ;
	void extract_frame_to_list (void) ;
} ;

}					// end of namespace casan
#endif
