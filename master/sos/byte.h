/**
 * @file byte.h
 * @brief Some byte-handling definitions
 */

#ifndef SOS_BYTE_H
#define SOS_BYTE_H

/** extract high-order byte from a 16-bits word */
#define	BYTE_HIGH(n)	(((n) & 0xff00) >> 8)

/** extract low-order byte from a 16-bits word */
#define	BYTE_LOW(n)	((n) & 0xff)

/** construct a 16-bits word from two bytes */
#define	INT16(h,l)	((h) << 8 | (l))

/** print a byte to an output stream */
#define	PRINT_HEX_DIGIT(os,c)	do { char d = (c) & 0xf ; d =  d < 10 ? d + '0' : d - 10 + 'a' ; (os) << d ; } while (false)

#endif
