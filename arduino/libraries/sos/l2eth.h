#ifndef SOS_L2ETH_H
#define	SOS_L2ETH_H

#include "Arduino.h"
#include "l2.h"

#define ETHADDRLEN 6

class l2addr_eth: public l2addr
{
	public:
		l2addr_eth () ;				// constructor
		l2addr_eth (const char *) ;		// constructor
		l2addr_eth (const l2addr_eth &) ;	// copy constructor
		l2addr_eth &operator= (const l2addr_eth &) ; // copy assignment

		~l2addr_eth () ;			// destructor

		bool operator== (const l2addr &) ;
		bool operator!= (const l2addr &) ;
		bool operator!= (const unsigned char* mac_addr);

		void set_addr(const unsigned char* mac_addr);
		unsigned char * get_raw_addr(void) ;

} ;

extern l2addr_eth l2addr_eth_broadcast ;

#endif
