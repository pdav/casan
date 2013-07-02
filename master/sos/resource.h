#ifndef SOS_RESOURCE_H
#define	SOS_RESOURCE_H

namespace sos {

class option ;
class mesg ;

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
	std::vector <option> pathopt_ ;	// path a a series of SOS options

	struct attr_t
	{
	    std::string name ;
	    std::list <std::string> values ;
	} ;
	std::vector <attr_t> attributes_ ; // attribute list
} ;

}					// end of namespace sos
#endif
