#include <Arduino.h>

#include "debug.h"

Debug debug ;				// global variable

int freeMemory () ;

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
	Serial.println (F ("-------------------------------------------------------------------")) ;
	Serial.print (F ("\033[36mloop \033[00m ")) ;
	Serial.print (F ("\tfree mem = \033[36m")) ;
	Serial.print (freeMemory ()) ;
	Serial.println (F ("\033[00m")) ;

	next_ += interv_ ;
	action = true ;
    }

    return action ;
}

// MemoryFree library based on code posted here:
// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1213583720/15
//
// Extended by Matthew Murdoch to include walking of the free list.

extern unsigned int __heap_start;
extern void *__brkval;

/*
 * The free list structure as maintained by the 
 * avr-libc memory allocation routines.
 */
struct __freelist {
	size_t sz;
	struct __freelist *nx;
};

/* The head of the free list structure */
extern struct __freelist *__flp;

/* Calculates the size of the free list */
int freeListSize() {
	struct __freelist* current;
	int total = 0;

	for (current = __flp; current; current = current->nx) {
		total += 2; /* Add two bytes for the memory block's header  */
		total += (int) current->sz;
	}

	return total;
}

int freeMemory() {
	int free_memory;

	if ((int)__brkval == 0) {
		free_memory = ((int)&free_memory) - ((int)&__heap_start);
	} else {
		free_memory = ((int)&free_memory) - ((int)__brkval);
		free_memory += freeListSize();
	}
	return free_memory;
}
