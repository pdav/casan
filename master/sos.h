#ifndef	SOS_SOS_H
#define	SOS_SOS_H

#include <chrono>

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

#define	DEFAULT_SLAVE_TTL	3600000			// 1 hour

typedef long int slaveid_t ;

typedef long int slavettl_t ;

typedef	std::chrono::system_clock::time_point timepoint_t ;
typedef std::chrono::milliseconds duration_t ;

#define	DATE_TIMEOUT(ms)	(std::chrono::system_clock::now () + \
				    duration_t (ms))

#ifdef DEBUG
#define	D(s)	do { std::cout << s << "\n" ; } while (false)	// no ";"
#else
#define	D(s)	do { } while (false)				// no ";"
#endif

#endif
