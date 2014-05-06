/**
 * @file slave.h
 * @brief SOS slave class definition
 */

#ifndef SOS_SLAVE_H
#define	SOS_SLAVE_H

#include <memory>

namespace sos {

class sos ;
class msg ;
class l2net ;
class l2addr ;
class resource ;

/**
 * @brief SOS slave class
 *
 * This class describes a slave in the SOS system.
 */

class slave
{
    public:
	enum status_code
	{
	    SL_INACTIVE = 0,
	    SL_ASSOCIATING = 0,
	    SL_RUNNING,
	} ;

	void reset (void) ;		// reset state to inactive

	// Mutators
	void l2 (l2net *l2) 		{ l2_ = l2 ; }
	void addr (l2addr *a) 		{ addr_ = a ; }
	void slaveid (slaveid_t sid)	{ slaveid_ = sid ; }
	void mtu (int m)		{ mtu_ = m ; }
	void init_ttl (int t)		{ init_ttl_ = t ; }
	/** Handler for incoming requests */
	void handler (reply_handler_t h) { handler_ = h ; }

	// Accessors
	l2net *l2 (void) 		{ return l2_ ; }
	l2addr *addr (void) 		{ return addr_ ; }
	slaveid_t slaveid (void) 	{ return slaveid_ ; }
	int mtu (void) 			{ return mtu_ ; }
	enum status_code status (void) 	{ return status_ ; }
	int init_ttl (void) 		{ return init_ttl_ ; }

	resource *find_resource (const std::vector <std::string> &v) ;
	const std::vector <resource> &resource_list (void) ;

	// SOS protocol handling
	void process_sos (sos *e, std::shared_ptr <msg> m) ;

	friend std::ostream& operator<< (std::ostream &os, const slave &s) ;

    protected:
	reply_handler_t handler_ ;	// handler to process answers

	friend class sos ;

	// needed for operator<<
	timepoint_t next_timeout_ ;	// remaining ttl

    private:
	slaveid_t slaveid_ = 0 ;	// slave id
	int mtu_ = 0 ;			// slave MTU
	l2net *l2_ = nullptr ;		// l2 network this slave is on
	l2addr *addr_ = nullptr ;	// slave address
	int init_ttl_ = 0 ;		// initial ttl (in sec)
	enum status_code status_ = SL_INACTIVE;	// current status of slave
	std::vector <resource> reslist_ ;	// resource list

	// parse a resource list returned by this slave
	// at the slave auto-discovery time
	bool parse_resource_list (std::vector <resource> &rlist, const byte *b, int len) ;
} ;

}					// end of namespace sos
#endif
