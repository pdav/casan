#include "l2eth.h"

l2addr_eth l2addr_eth_broadcast ("ff:ff:ff:ff:ff:ff") ;

/******************************************************************************
 * l2addr_eth methods
 */

// constructor
l2addr_eth::l2addr_eth ()
{
	addr = (byte *) malloc( sizeof(byte) * ETHADDRLEN) ;
}

// constructor
l2addr_eth::l2addr_eth (const char *a)
{
	int i = 0 ;
	byte b = 0 ;

	addr = (byte *) malloc( sizeof(byte) * ETHADDRLEN) ;

	while (*a != '\0' && i < ETHADDRLEN)
	{
		if (*a == ':')
		{
			addr [i++] = b ;
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
				addr [i] = 0 ;
			break ;
		}
		a++ ;
	}
	if (i < ETHADDRLEN)
		addr [i] = b ;
}

// copy constructor
l2addr_eth::l2addr_eth (const l2addr_eth &x)
{
	addr = new byte [ETHADDRLEN] ;
	memcpy (addr, x.addr, ETHADDRLEN) ;
}

// assignment operator
l2addr_eth & l2addr_eth::operator = (const l2addr_eth &x)
{
	if (addr == x.addr)
		return *this ;

	if (addr == NULL)
		addr = new byte [ETHADDRLEN] ;
	memcpy (addr, x.addr, ETHADDRLEN) ;
	return *this ;
}

/******************************************************************************
 * l2addr_eth methods
 */

bool l2addr_eth::operator== (const l2addr &other)
{
	l2addr_eth *oe = (l2addr_eth *) &other ;
	return memcmp (this->addr, oe->addr, ETHADDRLEN) == 0 ;
}

bool l2addr_eth::operator!= (const l2addr &other)
{
	l2addr_eth *oe = (l2addr_eth *) &other ;
	return memcmp (this->addr, oe->addr, ETHADDRLEN) != 0 ;
}

bool l2addr_eth::operator!= (const unsigned char* mac_addr)
{
	return memcmp(this->addr, mac_addr, ETHADDRLEN) != 0;
}

void l2addr_eth::set_addr(const unsigned char* mac_addr) 
{
	memcpy(this->addr, mac_addr, ETHADDRLEN);
}
unsigned char * l2addr_eth::get_raw_addr(void) 
{
	return this->addr;
}

// destructor
l2addr_eth::~l2addr_eth ()
{
	free(addr) ;
}

