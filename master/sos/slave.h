#ifndef SOS_SLAVE_H
#define	SOS_SLAVE_H

namespace sos {

class sos ;
class msg ;
class l2net ;
class l2addr ;
class resource ;

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
	void l2 (l2net *l2) ;		// set l2 network
	void addr (l2addr *a) ;		// set address
	void slaveid (slaveid_t sid) ;	// set slave id
	void initttl (int t) ;		// set initial slave ttl

	// Accessors
	l2net *l2 (void) ;
	l2addr *addr (void) ;
	slaveid_t slaveid (void) ;
	enum status_code status (void) ;
	int initttl (void) ;

	// SOS protocol handling
	void process_sos (sos *e, msg *m) ;

	void handler (reply_handler_t h) ;// handler for incoming requests

	friend std::ostream& operator<< (std::ostream &os, const slave &s) ;

    protected:
	// process a message from this slave
	// data is NULL if this is timeout
	void process (void *data, int len) ;

	reply_handler_t handler_ ;	// handler to process answers

	friend class sos ;

	// needed for operator<<
	timepoint_t next_timeout_ ;	// remaining ttl

    private:
	slaveid_t slaveid_ ;		// slave id
	l2net *l2_ ;			// l2 network this slave is on
	l2addr *addr_ ;			// slave address
	int initttl_ ;			// initial ttl (in sec)
	enum status_code status_ ;	// current status of slave
	std::vector <resource> reslist_ ;	// resource list

	// parse a resource list returned by this slave
	// at the slave auto-discovery time
	bool parse_resource_list (const byte *b, int len) ;
} ;

}					// end of namespace sos
#endif
