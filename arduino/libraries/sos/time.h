#ifndef __TIME_H__
#define __TIME_H__

#include "Arduino.h"
#include "defs.h"

#define MAX_LONGINT		4294967295

/*******
 * this class handles time
 */
class time {
	public:
		time();
		time(time &t);
		~time();

		bool operator< (time &t);
		time &operator= (time &t);

		void add(time &t);
		void add(unsigned long time);
		static long diff(time &a, time &b);
		void cur(void);			// set _millis to current value of millis()

		void print();

	private:
		unsigned long int _millis;		// val in milliseconds ;
		unsigned long int _restart;		// nb restart to 0 (function millis())
};

#endif
