#ifndef	GLOBAL_H
#define	GLOBAL_H

#include <chrono>
#include <cstdint>

// number of elements in an array
#define NTAB(t)		(int (sizeof (t)/sizeof (t)[0]))

// http://stackoverflow.com/questions/3366812/linux-raw-ethernet-socket-bind-to-specific-protocol
#define	ETHTYPE_SOS	0x88b5		// public use for prototype

#define	MAXBUF		1024

typedef std::uint8_t byte ;
typedef std::uint32_t uint32 ;

typedef long int slaveid_t ;
typedef long int sostimer_t ;

typedef	std::chrono::system_clock::time_point timepoint_t ;
typedef std::chrono::milliseconds duration_t ;

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
