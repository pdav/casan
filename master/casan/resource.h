/**
 * @file resource.h
 * @brief Resource class interface
 */

#ifndef CASAN_RESOURCE_H
#define	CASAN_RESOURCE_H

namespace casan {

class option ;
class mesg ;

/**
 * @brief An object of class Resource represents a resource which
 *	is provided by an CASAN slave.
 *
 * This class represents a resource. A resource has:
 * - an URL (vector of path components)
 * - some attributes: name, title, etc. which are stored as a
 *	list of (key, val) where the key is the name of the
 *	attribute (e.g. `name`, `title`, `ct`) and val is a list
 *	of associated values (`temp`, `Temperature`, `0`).
 */

class resource
{
    public:
	resource (const std::string path) ;	// constructor

	bool operator== (const std::string path) ;
	bool operator!= (const std::string path) ;
	bool operator== (const std::vector <std::string> &vpath) ;
	bool operator!= (const std::vector <std::string> &vpath) ;
	bool operator== (const std::vector <option> &pathopt) ;
	bool operator!= (const std::vector <option> &pathopt) ;

	// Mutators
	void add_attribute (const std::string name, const std::string value) ;
	void add_to_message (msg &m) ;

	// Accessors
	// return attribute values (or NULL if not found) for an attribute name
	std::list <std::string> *attribute (const std::string name) ;

	friend std::ostream& operator<< (std::ostream &os, const resource &r) ;

    private:
	std::vector <std::string> vpath_ ;
	std::vector <option> pathopt_ ;	// path a a series of CASAN options

	struct attr_t
	{
	    std::string name ;
	    std::list <std::string> values ;
	} ;
	std::vector <attr_t> attributes_ ; // attribute list
} ;

}					// end of namespace casan
#endif
