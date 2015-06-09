/**
 * @file token.h
 * @brief token class interface
 */

#ifndef CASAN_TOKEN_H
#define CASAN_TOKEN_H

#include "defs.h"

/**
 * @brief An object of class Token represents a token
 *
 * This class represents a token, i.e. a string whose length is
 * limited to 8 bytes.
 */

class Token
{
    public:
	Token () ;				// constructor
	Token (char *str) ;			// constructor (\0 terminated)
	Token (uint8_t *val, size_t len) ;	// constructor

	bool operator== (const Token &t) ;
	bool operator!= (const Token &t) ;

	void reset (void) ;
	void print (void) ;

    protected:
	int toklen_ ;
	uint8_t token_ [COAP_MAX_TOKLEN] ;	// no terminating \0 (raw array)

	friend class Msg;

    private:
} ;
#endif
