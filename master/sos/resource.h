#ifndef SOS_RESOURCE_H
#define	SOS_RESOURCE_H

namespace sos {

class option ;

class resource
{
    public:
	resource (const std::string path) ;	// constructor

	int operator== (const std::string path) ;
	int operator!= (const std::string path) ;
	int operator== (const std::vector <option> &pathopt) ;
	int operator!= (const std::vector <option> &pathopt) ;

	// Mutators
	void add_attribute (const std::string name, const std::string value) ;

	// Accessors
	// return attribute values (or NULL if not found) for an attribute name
	std::list <std::string> *attribute (const std::string name) ;

	friend std::ostream& operator<< (std::ostream &os, const resource &r) ;

    private:
	std::string path_ ;		// resource name / path
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
