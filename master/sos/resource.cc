/*
 * Resource representation
 *
 * Each slave has some resources, once it is discovered by the
 * master.
 */

#include <iostream>
#include <cstring>
#include <vector>

#include "global.h"

#include "utils.h"

#include "l2.h"
#include "msg.h"
#include "slave.h"
#include "option.h"
#include "sos.h"
#include "resource.h"

namespace sos {

/*
 * Constructor and destructor
 */

resource::resource (const std::string path)
{
    vpath_ = split_path (path) ;
    for (auto &p : vpath_)
	pathopt_.push_back (option (option::MO_Uri_Path, p.c_str (), p.length ())) ;
}

/******************************************************************************
 * Debug
 */

std::ostream& operator<< (std::ostream &os, const resource &r)
{
    os << join_path (r.vpath_) ;
    for (auto &a : r.attributes_)
	for (auto &v : a.values)
	    os << ";" << a.name << "=\"" << v << "\"" ;
    os << "\n" ;
    return os ;
}

/******************************************************************************
 * Resource matching
 */

int resource::operator== (const std::string path)
{
    std::vector <std::string> v = split_path (path) ;

    return *this == v ;
}

int resource::operator!= (const std::string path)
{
    return ! (*this == path) ;
}

int resource::operator== (const std::vector <std::string> &vpath)
{
    int r ;
 
    if (vpath.size () == vpath_.size ())
    {
	r = 0 ;				// by default : equal
	for (std::string::size_type i = 0 ; i < vpath.size () ; i++)
	{
	    if (vpath [i] != vpath_ [i])
	    {
		r = 1 ;			// not equal
		break ;
	    }
	}
    }
    else r = 1 ;

    return r ;
}

int resource::operator!= (const std::vector <std::string> &vpath)
{
    return ! (*this == vpath) ;
}

int resource::operator== (const std::vector <option> &pathopt)
{
    unsigned int sz = pathopt_.size () ;
    int r = 0 ;

    if (sz == pathopt.size ())
    {
	r = 1 ;
	for (unsigned int i = 0 ; i < sz ; i++)
	{
	    if (pathopt_ [i] != pathopt [i])
	    {
		r = 0 ;
		break ;
	    }
	}
    }
    return r ;
}

int resource::operator!= (const std::vector <option> &pathopt)
{
    return ! (*this == pathopt) ;
}

/******************************************************************************
 * Accessors
 */

std::list <std::string> *resource::attribute (const std::string name)
{
    for (auto &a : attributes_)
    {
	if (a.name == name)
	    return & (a.values) ;
    }
    return nullptr ;
}

/******************************************************************************
 * Mutators
 */

void resource::add_attribute (const std::string name, const std::string value)
{
    bool found = false ;

    for (auto &a : attributes_)
    {
	if (a.name == name)
	{
	    a.values.push_back (value) ;
	    found = true ;
	    break ;
	}
    }
    if (! found)
    {
	attributes_.push_back (attr_t ()) ;
	attributes_.back ().name = name ;
	attributes_.back ().values.push_back (value) ;
    }
}

void resource::add_to_message (msg &m)
{
    for (auto &o : pathopt_)
	m.pushoption (o) ;
}

}					// end of namespace sos
