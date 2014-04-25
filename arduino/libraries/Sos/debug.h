/**
 * @file debug.h
 * @brief Debug class interface
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

/*
 * Some debug fancy features
 */

#include "Arduino.h"
#include "defs.h"

/**
 * @brief Debug facility
 *
 * This class give some handy methods to ease debugging.
 *
 * At this moment, the only facility is to display, at fixed intervals,
 * a heartbeat type message containing the amount of free memory. This
 * allows to monitor the application, provided that the method be called
 * in the `loop` function of the application.
 */

class Debug
{
    public:
	void start (int interval) ;	// interval between actions, in seconds
	bool heartbeat (void) ;		// true if action done

    private:
	unsigned long int interv_ ;	// interval between actions
	unsigned long int next_ ;	// next action
} ;

#endif
