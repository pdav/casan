/*
 * SOS common definitions
 *
 */

// number of elements in an array
#define NTAB(t)		(sizeof (t)/sizeof (t)[0])


// http://stackoverflow.com/questions/3366812/linux-raw-ethernet-socket-bind-to-specific-protocol
#define	ETHTYPE_SOS	0x88b5		// public use for prototype

// Ethernet address length
#define	ETHADDRLEN	6
