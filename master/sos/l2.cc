#include <iostream>

#include "global.h"
#include "l2.h"

namespace sos {

std::ostream& operator<< (std::ostream &os, const l2addr &a)
{
    a.print (os) ;
    return os ;
}

int l2net::mtu (void)
{
    return mtu_ ;
}

int l2net::maxlatency (void)
{
    return maxlatency_ ;
}

}					// end of namespace sos
