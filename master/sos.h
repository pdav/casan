#ifndef	SOS_SOS_H
#define	SOS_SOS_H

#define	MAXBUF		1024

typedef unsigned char byte ;
typedef unsigned int uint32 ;

// ME:          packet addressed to me
// BCAST:       packet broadcasted
// NONE:        not a packet (or not for me)
typedef enum pktype { PK_ME, PK_BCAST, PK_NONE } pktype_t ;

// delays (in milliseconds)
#define	INTERVAL_HELLO		10000
#define	DELAY_FIRST_HELLO	3000

typedef long slaveid_t ;

#define	DATE_TIMEOUT(ms)	(std::chrono::system_clock::now () + \
				    std::chrono::milliseconds (ms))

#ifdef DEBUG
#define	D(s)	do { std::cout << s << "\n" ; } while (false)	// no ";"
#else
#define	D(s)	do { } while (false)				// no ";"
#endif

#endif
