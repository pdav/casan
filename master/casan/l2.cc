/**
 * @file l2.cc
 * @brief Generic l2net methods
 */

#include <iostream>

#include "global.h"
#include "l2.h"

namespace casan {

/**
 * @brief Print L2 addresses
 *
 * This function is based on a hack in order to generically
 * print L2 addresses, since `operator<<` cannot be overloaded
 * with l2addr_xxx classes.
 */

std::ostream& operator<< (std::ostream &os, const l2addr &a)
{
    a.print (os) ;
    return os ;
}

}					// end of namespace casan
