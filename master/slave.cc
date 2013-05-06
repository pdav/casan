/*
 * Slave representation
 *
 * Each slave has a unique representation in the SOS system.
 */

#include <iostream>

#include "slave.h"

/*
 * Constructor and destructor
 */

slave::slave ()
{
    reset () ;
}

slave::~slave ()
{
}

void slave::reset (void)
{
    status_ = SL_INACTIVE ;
    slaveid_ = 0 ;
    l2_ = 0 ;
    addr_ = 0 ;
    handler_ = 0 ;
}

/******************************************************************************
 * Accessors
 */

l2net *slave::l2 (void)
{
    return l2_ ;
}

l2addr *slave::addr (void)
{
    return addr_ ;
}

int slave::ttl (void)
{
    return ttl_ ;
}

slaveid_t slave::slaveid (void)
{
    return slaveid_ ;
}

/******************************************************************************
 * Mutators
 */

void slave::l2 (l2net *l2)
{
    l2_ = l2 ;
}

void slave::addr (l2addr *a)
{
    addr_ = a ;
}

void slave::ttl (int ttl)
{
    ttl_ = ttl ;
}

void slave::slaveid (slaveid_t sid)
{
    slaveid_ = sid ;
}

void slave::handler (reply_handler_t h)
{
    handler_ = h ;
}

/*
 * Process an event: either a message or a timeout from the slave
 */

void slave::process (void *data, int len)
{
    if (data)
    {
	// regular message
	std::cout << "received a message of len " << len << " bytes\n" ;
    }
    else
    {
	// timeout
	std::cout << "got a timeout\n" ;
    }
}
