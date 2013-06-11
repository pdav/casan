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
    enum { s_start, s_loop } state = s_start ;
    std::string component ;

    path_ = path ;
    component = "" ;
    for (auto &c : path)
    {
	switch (state)
	{
	    case s_start :
		if (c != '/')
		{
		    component += c ;
		    state = s_loop ;
		}
		break ;
	    case s_loop :
		if (c == '/')
		{
		    pathopt_.push_back (option (option::MO_Uri_Path, component.c_str (), component.length ())) ;
		    state = s_start ;
		}
		else component += c ;
		break ;
	}
    }
    if (component != "")
	pathopt_.push_back (option (option::MO_Uri_Path, component.c_str (), component.length ())) ;
}

/******************************************************************************
 * Debug
 */

std::ostream& operator<< (std::ostream &os, const resource &r)
{
    os << "resource <" << r.path_ << ">" ;
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
    return path_ == path ;
}

int resource::operator!= (const std::string path)
{
    return ! (*this == path) ;
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

}					// end of namespace sos
