#include <string>
#include <vector>
#include <chrono>
#include <cstdlib>

#include "global.h"
#include "utils.h"

namespace sos {

/******************************************************************************
 * Some utility functions
 */

/*
 * returns an integer peuso-random value in [0..n[
 */

int random_value (int n)
{
    return std::rand () % n ;
}


/*
 * returns a random delay between [0...maxmilli] milliseconds
 */

duration_t random_timeout (int maxmilli)
{
    int delay ;

    delay = random_value (maxmilli + 1) ;
    return duration_t (delay) ;
}

/*
 * Split a path into components, skipping multiple "/" characters
 */

std::vector <std::string> split_path (const std::string &path)
{
    enum { s_start, s_middle } state = s_start ;
    std::vector <std::string> v ;

    for (auto &c : path)
    {
	switch (state)
	{
	    case s_start :
		if (c != '/')
		{
		    v.push_back ("") ;
		    v.back ().push_back (c) ;
		    state = s_middle ;
		}
		break ;
	    case s_middle :
		if (c != '/')
		    v.back ().push_back (c) ;
		else state = s_start ;
		break ;
	}
    }
    return v;
}

/*
 * Join components to give a path
 */

std::string join_path (const std::vector <std::string> &vpath)
{
    std::string p ;

    if (vpath.size () == 0)
	p = "/" ;
    else
    {
	p = "" ;
	for (auto &component : vpath)
	    p += "/" + component ;
    }
    return p ;
}

}					// end of namespace sos
