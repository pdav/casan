#ifndef __DEBUG_H__
#define __DEBUG_H__

/*
 * Some debug fancy features
 */

#include "Arduino.h"
#include "defs.h"

class Debug
{
    public:
	void start (int interval) ;	// interval between actions, in seconds
	bool heartbeat (void) ;		// true if action done

    private:
	long int interv_ ;		// interval between actions
	long int next_ ;		// next action
} ;

extern Debug debug ;

#endif
