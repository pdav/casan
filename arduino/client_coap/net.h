#ifndef SOS_NET_H
#define	SOS_NET_H

enum l2type
{
    L2_ETH = 0,
    L2_802154
} ;
typedef enum l2type l2type_t ;

typedef struct l2desc l2desc_t ;

struct sos_net
{
    l2type_t type ;
    l2desc_t *desc ;
} ;
typedef struct sos_net sos_net_t ;


#endif
