#ifndef SOS_BYTE_H
#define SOS_BYTE_H

#define	BYTE_HIGH(n)	(((n) & 0xff00) >> 8)
#define	BYTE_LOW(n)	((n) & 0xff)
#define	INT16(h,l)	((h) << 8 | (l))

#endif
