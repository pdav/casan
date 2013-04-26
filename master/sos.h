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

#endif
