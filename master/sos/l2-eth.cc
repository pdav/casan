/**
 * @file l2-eth.cc
 * @brief L2addr_eth and l2net_eth class implementations
 */

#include <iostream>
#include <cstring>

#include <sys/types.h>
#include <unistd.h>

#ifdef USE_PF_PACKET
#include <sys/socket.h>
#include <netpacket/packet.h>
#endif
#ifdef USE_PCAP
#include <pcap/pcap.h>
#endif

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "global.h"

#include "l2.h"
#include "l2-eth.h"
#include "byte.h"

namespace sos {

/******************************************************************************
 * l2addr_eth methods
 */

// default constructor
l2addr_eth::l2addr_eth ()
{
}

/** Constructor with an address given as a string
 *
 * This constructor is used to initialize an address with a
 * string such as "`01:02:03:04:05:06`".
 *
 * @param a address to be parsed
 */

l2addr_eth::l2addr_eth (const char *a)
{
    int i = 0 ;
    byte b = 0 ;

    while (*a != '\0' && i < ETHADDRLEN)
    {
	if (*a == ':')
	{
	    addr_ [i++] = b ;
	    b = 0 ;
	}
	else if (isxdigit (*a))
	{
	    byte x ;
	    char c ;

	    c = tolower (*a) ;
	    x = isdigit (c) ? (c - '0') : (c - 'a' + 10) ;
	    b = (b << 4) + x ;
	}
	else
	{
	    for (i = 0 ; i < ETHADDRLEN ; i++)
		addr_ [i] = 0 ;
	    break ;
	}
	a++ ;
    }
    if (i < ETHADDRLEN)
	addr_ [i] = b ;
}

// copy constructor
l2addr_eth::l2addr_eth (const l2addr_eth &l)
{
    *this = l ;
}

// copy assignment constructor
l2addr_eth &l2addr_eth::operator= (const l2addr_eth &l)
{
    if (this != &l)
	*this = l ;
    return *this ;
}

/**
 * @brief Ethernet broadcast address
 */

l2addr_eth l2addr_eth_broadcast ("ff:ff:ff:ff:ff:ff") ;

/**
 * @brief Print address
 *
 * This function is a hack needed for l2::operator<< overloading
 */

void l2addr_eth::print (std::ostream &os) const
{
    for (int i = 0  ; i < ETHADDRLEN ; i++)
    {
	if (i > 0)
	    os << ":" ;
	PRINT_HEX_DIGIT (os, addr_ [i] >> 4) ;
	PRINT_HEX_DIGIT (os, addr_ [i]     ) ;
    }
}

bool l2addr_eth::operator== (const l2addr &other)
{
    l2addr_eth *oe = (l2addr_eth *) &other ;
    return std::memcmp (this->addr_, oe->addr_, ETHADDRLEN) == 0 ;
}

bool l2addr_eth::operator!= (const l2addr &other)
{
    return ! (*this == other) ;
}

/******************************************************************************
 * l2net_eth methods
 */

/**
 * @brief Initialize an Ethernet network access
 *
 * Initialize the Ethernet network access with needed constants.
 *
 * @param iface Ethernet interface name (eth0, etc.)
 * @param ethertype Ethernet frame type
 * @return -1 if initialization fails (`errno` is set)
 */

int l2net_eth::init (const std::string iface, int ethertype)
{
#if defined (USE_PF_PACKET)
    struct sockaddr_ll sll ;

    mtu_ = ETHMTU ;
    maxlatency_ = ETHMAXLATENCY ;
    ethertype_ = ethertype ;

    /* Get interface index */

    ifidx_ = if_nametoindex (iface.c_str ()) ;
    if (ifidx_ == 0)
	return -1 ;

    /* Open socket */

    fd_ = socket (PF_PACKET, SOCK_DGRAM, htons (ethertype_)) ;
    if (fd_ == -1)
	return -1 ;

    /* Bind the socket to the interface */

    std::memset (&sll, 0, sizeof sll) ;
    sll.sll_family = AF_PACKET ;
    sll.sll_protocol = htons (ethertype_) ;
    sll.sll_ifindex = ifidx_ ;
    if (bind (fd_, (struct sockaddr *) &sll, sizeof sll) == -1)
    {
	close (fd_) ;
	return -1 ;
    }

    return 0 ;
#elif defined (USE_PCAP)
    struct bpf_program *bpfp ;
    char buf [MAXBUF] ;

    fd_ = pcap_create (iface.c_str (), errbuf_) ;
    if (fd_ == NULL)
	return -1 ;

    snprintf (buf, sizeof buf, "ether proto 0x%x", ethertype_) ;
    if (pcap_compile (fd_, bpfp, buf, 1, PCAP_NETMASK_UNKNOWN) != 0)
    {
	pcap_close (fd_) ;
	return -1 ;
    }

    if (pcap_setfilter (fd_, bpfp) != 0)
    {
	pcap_close (fd_) ;
	return -1 ;
    }
    pcap_freecode (bpfp) ;

    if (pcap_activate (fd_) != 0)
    {
	pcap_close (fd_) ;
	return -1 ;
    }

    return 0 ;
#else
    if (iface == "" && ethertype == 0)
	return -1 ;
    return 0 ;
#endif
}

/**
 * @brief Closes access to network
 */

void l2net_eth::term (void)
{
#if defined (USE_PF_PACKET)
    close (fd_) ;
#elif defined (USE_PCAP)
    pcap_close (fd_) ;
#endif
}

/**
 * @brief Send a frame to the given destination address
 *
 * @return number of bytes sent
 */

int l2net_eth::send (l2addr *daddr, void *data, int len)
{
    int r ;

#if defined (USE_PF_PACKET)
    struct sockaddr_ll sll ;
    l2addr_eth *a = (l2addr_eth *) daddr ;
    byte *buf ;

    /*
     * Ethernet specific: add message length at the beginnin of the payload
     */

    len += 2 ;
    buf = new byte [len] ;
    buf [0] = BYTE_HIGH (len) ;
    buf [1] = BYTE_LOW (len) ;
    std::memcpy (buf + 2, data, len - 2) ;

    /*
     * Send Ethernet raw packet
     */

    std::memset (&sll, 0, sizeof sll) ;
    sll.sll_family = AF_PACKET ;
    sll.sll_protocol = htons (ethertype_) ;
    sll.sll_halen = ETHADDRLEN ;
    sll.sll_ifindex = ifidx_ ;
    std::memcpy (sll.sll_addr, a->addr_, ETHADDRLEN) ;

    r = sendto (fd_, buf, len, 0, (struct sockaddr *) &sll, sizeof sll) ;
#elif defined (USE_PCAP)
    r = 0 ;
#else
    /*
     * Ethernet not supported: calm down gcc and use all arguments in
     * a big non-sense
     */

    if (daddr == NULL && data == NULL && len == 0)
	r = -1 ;
    r = -1 ;
#endif
    return r ;
}

/**
 * @brief Send a frame to the broadcast address
 *
 * @return number of bytes sent
 */

int l2net_eth::bsend (void *data, int len)
{
    return send (&l2addr_eth_broadcast, data, len) ;
}

/**
 * @brief Return the broadcast address for this network
 */

l2addr *l2net_eth::bcastaddr (void)
{
    return &l2addr_eth_broadcast ;
}

pktype_t l2net_eth::recv (l2addr **saddr, void *data, int *len)
{
    pktype_t pktype ;

#if defined (USE_PF_PACKET)
    struct sockaddr_ll sll ;
    socklen_t ssll ;
    l2addr_eth **a = (l2addr_eth **) saddr ;
    int r ;
    pktype_t pktype ;
    byte *buf ;
    int lenlen = *len + 2 ;

    /*
     * Ethernet specific : use additional 2 bytes for length in
     * front of the payload
     */


    buf = new byte [lenlen] ;

    std::memset (&sll, 0, sizeof sll) ;
    sll.sll_family = AF_PACKET ;
    sll.sll_ifindex = ifidx_ ;
    sll.sll_protocol = htons (ETHTYPE_SOS) ;
    sll.sll_halen = ETHADDRLEN ;

    ssll = sizeof sll ;

    r = recvfrom (fd_, buf, lenlen, 0, (struct sockaddr *) &sll, &ssll) ;
    if (r == -1)
    {
	*a = nullptr ;
	pktype = PK_NONE ;
    }
    else
    {
	*a = new l2addr_eth ;

	/*
	 * Remove Ethernet specific length
	 */

	*len = INT16 (buf [0], buf [1]) - 2 ;	// true length
	std::memcpy (data, buf + 2, r - 2) ;
	delete buf ;

	// *len = r ;
	switch (sll.sll_pkttype)
	{
	    case PACKET_HOST :
		pktype = PK_ME ;
		break ;
	    case PACKET_BROADCAST :
	    case PACKET_MULTICAST :
		pktype = PK_BCAST ;
		break ;
	    case PACKET_OTHERHOST :
	    case PACKET_OUTGOING :
	    default :
		pktype = PK_NONE ;
		break ;
	}

	std::memcpy ((*a)->addr_, sll.sll_addr, ETHADDRLEN) ;
    }

#elif defined (USE_PCAP)
    struct pcap_pkthdr pkthdr ;
    const u_char *d ;

    d = pcap_next (fd_, &pkthdr) ;
    if (d == NULL)
    {
	pktype = PK_NONE ;
    }
    else
    {
	if (*len >= (int) pkthdr.caplen)
	    *len = (int) pkthdr.caplen ;
	std::memcpy (data, d, *len) ;

	pktype = PK_ME ;
    }

#else
    /*
     * Ethernet not supported: calm down gcc and use all arguments in
     * a big non-sense
     */

    if (saddr == NULL && data == NULL && len == NULL)
	pktype = PK_NONE ;
    pktype = PK_NONE ;

#endif
    return pktype ;
}

}					// end of namespace sos
