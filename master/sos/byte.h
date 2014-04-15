#ifndef SOS_BYTE_H
#define SOS_BYTE_H

#define	BYTE_HIGH(n)	(((n) & 0xff00) >> 8)
#define	BYTE_LOW(n)	((n) & 0xff)
#define	INT16(h,l)	((h) << 8 | (l))

#define	PRINT_HEX_DIGIT(os,c)	do { char d = (c) & 0xf ; d =  d < 10 ? d + '0' : d - 10 + 'a' ; (os) << d ; } while (false)

#endif
