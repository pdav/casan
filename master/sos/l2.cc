#include "global.h"
#include "l2.h"

namespace sos {

int l2net::mtu (void)
{
    return mtu_ ;
}

int l2net::maxlatency (void)
{
    return maxlatency_ ;
}

}					// end of namespace sos
