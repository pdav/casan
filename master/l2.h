#ifndef	SOS_L2_H
#define	SOS_L2_H

typedef unsigned char byte ;

// ME:          packet addressed to me
// BCAST:       packet broadcasted
// NONE:        not a packet (or not for me)
typedef enum pktype { PK_ME, PK_BCAST, PK_NONE } pktype_t ;

class l2addr
{
    public:
    protected:
	byte *addr ;
} ;

class l2net
{
    public:
	virtual int	  init (const char *iface) = 0 ;
	virtual void	  term (void) = 0 ;
	virtual int	  getfd (void) ;
	virtual int	  send (l2addr *daddr, void *data, int len) = 0 ;
	virtual pktype_t  recv (l2addr *saddr, void *data, int *len) = 0 ;
    protected:
	int fd ;
} ;

#endif
