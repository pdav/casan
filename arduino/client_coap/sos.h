#ifndef SOS_H
#define SOS_H

#define	NTAB(tab)	(sizeof (tab) / sizeof (tab [0]))


// ME:		packet addressed to me
// BCAST:	packet broadcasted
// NONE:	not a packet (or not for me)
typedef enum pktype { PK_ME, PK_BCAST, PK_NONE } pktype_t ;

#endif
