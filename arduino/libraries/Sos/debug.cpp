#include "debug.h"

Debug debug ;				// global variable

void Debug::start (int interval)	// in seconds
{
    Serial.println (F ("\033[36mstart\033[00m")) ;
    interv_ = interval * 1000 ;		// in milliseconds
    next_ = millis () ;			// perform action immediately
}

bool Debug::heartbeat (void)
{
    bool action = false ;

    if (millis () > next_)
    {
	Serial.println (F ("-----------------------------------------------")) ;
	Serial.println (F ("\033[36mloop \033[00m ")) ;
	// PRINT_FREE_MEM ;
	next_ += interv_ ;
	action = true ;
    }

    return action ;
}
