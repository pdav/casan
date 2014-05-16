/**
 * @file cache.cc
 * @brief cache implementation
 */

#include <iostream>

#include "cache.h"

namespace sos {

/**
 * @brief Ask the cache for the reply to a request
 *
 * This method looks the cache for a reply to a given request.
 * The request must have been linked with the reply.
 * Cache cleanup (removing obsolete entries) is performed if needed.
 *
 * @param req a request
 * @result reply found or nullptr
 */

msgptr_t cache::get (msgptr_t req)
{
    std::unique_lock <std::mutex> lk (mtx_) ;
    msgptr_t r = nullptr ;
    timepoint_t now ;
    bool need_to_remove = false ;

    now = std::chrono::system_clock::now () ;

    /*
     * Look for the request in cache and check if a cache cleanup
     * must be performed.
     */

    for (auto &e : cache_)
    {
	if (now > e.expire)
	{
	    need_to_remove = true ;
	    D ("Cache: expiring " << *(e.request)) ;
	}
	else
	{
	    if (req->cache_match (e.request))
	    {
		D ("Cache: found " << *(e.request)) ;
		r = e.request ;
		break ;
	    }
	}
    }

    /*
     * Perform cache cleanup
     */

    if (need_to_remove)
    {
	cache_.remove_if (
		[now]
		(const entry &e)
		{
		    return now > e.expire ;
		}
	    ) ;
    }

    return r ;
}


/**
 * @brief Add a request to the cache
 *
 * Once a reply has been received, add the correlated request to
 * the cache. Request and reply must have already been linked
 * together. If the reply does not indicate a cacheable status,
 * the pair of messages is not inserted in the cache.
 *
 * @param req a request
 */

void cache::add (msgptr_t req)
{
    msgptr_t rep ;
    long int maxage ;

    rep = req->reqrep () ;
    if (rep != nullptr)
    {
	maxage = rep->max_age () ;
	if (maxage != -1)
	{
	    std::unique_lock <std::mutex> lk (mtx_) ;
	    timepoint_t now ;
	    entry e ;

	    D ("Cache: adding " << *req << " for " << maxage << " sec") ;

	    now = std::chrono::system_clock::now () ;
	    e.expire = now + duration_t (maxage * 1000) ;
	    e.request = req ;
	    cache_.push_back (e) ;
	}
    }
}

}					// end of namespace sos
