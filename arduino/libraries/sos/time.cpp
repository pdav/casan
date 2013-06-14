#include "time.h"

time::time()
{
	_millis = millis();
	_restart = 0;
}

time::time(time &t)
{
	_millis = t._millis;
	_restart = t._restart;
}

time::~time()
{
}

bool time::operator<(time &t)
{
	return _restart < t._restart || 
		_restart == t._restart && 
		_millis < t._millis;
}

time &time::operator= (time &t)
{
	if (this != &t)
	{
		memcpy (this, &t, sizeof *this) ;
	}
	return *this ;
}

void time::add(unsigned long time)
{
	if(time + _millis > MAX_LONGINT)
	{
		_millis = time - (MAX_LONGINT - _millis);
		_restart++;
	}
	else
	{
		_millis += time;
	}
}

void time::add(time &t)
{
	if(_millis + t._millis > MAX_LONGINT)
	{
		_restart++;
	}
	_millis = (_millis + t._millis) % MAX_LONGINT;

	_restart += t._restart;
}

long time::diff(time &a, time &b)
{
	long diff = 0;

	/*
	if(a < b)
	{

		if( a._restart != b._restart)
		{
			diff += MAX_LONGINT - a._millis;
			diff += b._millis;
		}
		else
		{
			diff += b._millis - a._millis;
		}

	}
	else
	{
	*/

		if( a._restart != b._restart)
		{
			diff += MAX_LONGINT - b._millis;
			diff += a._millis;
		}
		else
		{
			diff += (a._millis - b._millis);
		}

	//}

	PRINT_DEBUG_STATIC("\033[31m DIFF : \033[00m ");
	PRINT_DEBUG_DYNAMIC(diff);
	delay(100);
	return diff;
}

// set _millis to current value of millis()
void time::cur(void)		
{
	long int ms = millis();
	if(_millis > ms)
	{
		PRINT_DEBUG_STATIC("\033[31m _millis > ms LOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOL");
		_restart++;
	}
	_millis = ms;
}

void time::print()
{
	Serial.print(F("\033[33m_millis \033[00m: "));
	Serial.print(_millis);
	Serial.print(F(" \033[33m_restart \033[00m: "));
	Serial.println(_restart);
}
