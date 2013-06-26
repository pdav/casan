#ifndef	SOS_L2_H
#define	SOS_L2_H

#include "util.h"

class l2addr
{
	public:
		virtual bool operator== (const l2addr &other) = 0 ;
		virtual bool operator!= (const l2addr &other) = 0 ;
		virtual bool operator!= (const unsigned char* mac_addr) = 0 ;
	protected:
		byte *addr = NULL;
} ;

#endif
