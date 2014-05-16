/**
 * @file l2-eth.h
 * @brief l2addr and l2net specializations for Ethernet
 *
 * This file should be compiled with the `-DUSE_PF_PACKET` (Linux) or
 * the `-DUSE_PCAP` (FreeBSD, not available yet) flag.
 * If neither of these 2 flags is provided, SOS does not support
 * Ethernet access.
 */

#ifndef SOS_L2ETH_H
#define	SOS_L2ETH_H

#include "l2.h"

#ifdef USE_PCAP
#include <pcap/pcap.h>
#endif

// Ethernet address length
#define	ETHADDRLEN	6
// Ethernet default MTU
#define	ETHMTU		1536
// Ethernet maximum latency (ms). Even a very conservative value is far
// more realistic than the default CoAP value (100 s)
// #define	ETHMAXLATENCY	10
#define	ETHMAXLATENCY	50


namespace sos {

/**
 * @brief Specialization of the l2addr abstract class for Ethernet
 *
 * This class provides real methods (specified in l2addr virtual
 * class) for Ethernet addresses.
 */

class l2addr_eth: public l2addr
{
    public:
	l2addr_eth () ;				// default constructor
	l2addr_eth (const char *) ;		// constructor
	l2addr_eth (const l2addr_eth &l) ;	// copy constructor
	l2addr_eth &operator= (const l2addr_eth &l) ;	// copy assignment constructor

	bool operator== (const l2addr &) ;
	bool operator!= (const l2addr &) ;

	friend class l2net_eth ;

    protected:
	void print (std::ostream &os) const ;

    private:
	byte addr_ [ETHADDRLEN] ;
} ;

extern l2addr_eth l2addr_eth_broadcast ;

/**
 * @brief Specialization of the l2net abstract class for Ethernet
 *
 * This class provides real methods (specified in l2net virtual
 * class) for Ethernet networks.
 */

class l2net_eth: public l2net
{
    public:
	~l2net_eth () {} ;
	int init (const std::string iface, int mtu, int ethertype) ;
	void term (void) ;
	int send (l2addr *daddr, void *data, int len) ;
	int bsend (void *data, int len) ;
	pktype_t recv (l2addr **saddr, void *data, int *len) ;
	l2addr *bcastaddr (void) ;

    private:
#ifdef USE_PF_PACKET
	int ifidx_ ;			// interface index
	int fd_ ;			// socket descriptor
	int ethertype_  ;		// ethernet type
#endif
#ifdef USE_PCAP
	pcap_t *fd_ ;			// pcap descriptor
	char errbuf_ [PCAP_ERRBUF_SIZE] ;
#endif
} ;

}					// end of namespace sos
#endif
