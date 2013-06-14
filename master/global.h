#ifndef	GLOBAL_H
#define	GLOBAL_H

#include <chrono>

// number of elements in an array
#define NTAB(t)		(int (sizeof (t)/sizeof (t)[0]))


#define	MAXBUF		1024

typedef unsigned char byte ;
typedef unsigned int uint32 ;

typedef long int slaveid_t ;
typedef long int slavettl_t ;

typedef	std::chrono::system_clock::time_point timepoint_t ;
typedef std::chrono::milliseconds duration_t ;

// delays (in milliseconds)
#define	INTERVAL_HELLO		10000
#define	DELAY_FIRST_HELLO	3000

#define	DEFAULT_SLAVE_TTL	3600000			// 1 hour

#define	DATE_TIMEOUT_MS(ms)	(std::chrono::system_clock::now () + \
				    std::chrono::milliseconds (ms))
#define	DATE_TIMEOUT_S(s)	(std::chrono::system_clock::now () + \
				    std::chrono::seconds (s))

namespace sos {
    class msg ;
}

typedef void (*reply_handler_t) (sos::msg *request, sos::msg *reply) ;

#ifdef DEBUG
#define	D(s)	do { std::cout << s << "\n" ; } while (false)	// no ";"
#else
#define	D(s)	do { } while (false)				// no ";"
#endif

#endif
