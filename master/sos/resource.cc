/**
 * @file resource.cpp
 * @brief Resource class implementation
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

/**
 * @brief Constructor needs a path
 *
 * The path is splitted into components and stored in the
 * resource class.
 */

resource::resource (const std::string path)
{
    vpath_ = split_path (path) ;
    for (auto &p : vpath_)
	pathopt_.push_back (option (option::MO_Uri_Path, p.c_str (), p.length ())) ;
}

/**
 * @brief Dumps a resource to an output stream
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

bool resource::operator== (const std::string path)
{
    std::vector <std::string> v = split_path (path) ;

    return *this == v ;
}

bool resource::operator!= (const std::string path)
{
    return ! (*this == path) ;
}

bool resource::operator== (const std::vector <std::string> &vpath)
{
    bool r = false ;
 
    if (vpath.size () == vpath_.size ())
    {
	r = true ;			// by default : equal
	for (std::size_t i = 0 ; i < vpath.size () ; i++)
	{
	    if (vpath [i] != vpath_ [i])
	    {
		r = false ;
		break ;
	    }
	}
    }

    return r ;
}

bool resource::operator!= (const std::vector <std::string> &vpath)
{
    return ! (*this == vpath) ;
}

bool resource::operator== (const std::vector <option> &pathopt)
{
    unsigned int sz = pathopt_.size () ;
    bool r = false ;

    if (sz == pathopt.size ())
    {
	r = true ;
	for (unsigned int i = 0 ; i < sz ; i++)
	{
	    if (pathopt_ [i] != pathopt [i])
	    {
		r = false ;
		break ;
	    }
	}
    }
    return r ;
}

bool resource::operator!= (const std::vector <option> &pathopt)
{
    return ! (*this == pathopt) ;
}

/**
 * @brief Get attribute values for a given name
 *
 * @return List of strings representing the values
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

/**
 * @brief Add a value to an attribute
 *
 * @param name name of attribute
 * @param value single value to add to the value list
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

/**
 * @brief Add the resource path to a message
 *
 * Add the resource path to a message as some CoAP options
 *
 * @param m message to add the path to
 */

void resource::add_to_message (msg &m)
{
    for (auto &o : pathopt_)
	m.pushoption (o) ;
}

}					// end of namespace sos
