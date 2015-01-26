/**
 * @file cache.h
 * @brief cache interface
 */

#ifndef	CACHE_H
#define	CACHE_H

#include <list>
#include <memory>
#include <mutex>

#include "global.h"
#include "msg.h"

namespace casan {

/**
 * @brief CASAN cache interface
 *
 * The CASAN cache interface provides two basic operations: `add` and `get`.
 * The cache is 
 */

class cache
{
    public:
	msgptr_t get (msgptr_t req) ;
	void add (msgptr_t req) ;
    private:
	struct entry
	{
	    timepoint_t expire ;
	    msgptr_t request ;		// reply is linked with the request
	} ;
	std::list <entry> cache_ ;
	std::mutex mtx_ ;		// protect list access
} ;

}					// end of namespace casan
#endif
