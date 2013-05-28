#ifndef SOS_SLAVE_H
#define	SOS_SLAVE_H

class sos ;
class msg ;
class l2net ;
class l2addr ;

class slave
{
    public:
	enum status
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

	// Accessors
	l2net *l2 (void) ;
	l2addr *addr (void) ;
	slaveid_t slaveid (void) ;

	// SOS protocol handling
	void process_sos (sos *e, msg *m) ;

	void handler (reply_handler_t h) ;// handler for incoming requests

    protected:
	// process a message from this slave
	// data is NULL if this is timeout
	void process (void *data, int len) ;

	slaveid_t slaveid_ ;		// slave id
	enum status status_ ;		// current status of slave
	reply_handler_t handler_ ;	// handler to process answers

	friend class sos ;

    private:
	l2net *l2_ ;			// l2 network this slave is on
	l2addr *addr_ ;			// slave address
	timepoint_t next_timeout_ ;	// remaining ttl
} ;

#endif
