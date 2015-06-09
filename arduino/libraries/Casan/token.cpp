/**
 * @file token.cpp
 * @brief Token class implementation
 */

#include "token.h"

/******************************************************************************
 * Constructors and destructors
 */

/**
 * Default constructor
 *
 * The default constructor initializes option attributes
 */

Token::Token ()
{
    toklen_ = 0 ;
}

/**
 * Constructor with initialization string (terminated with \0)
 */

Token::Token (char *str)
{
    int i = 0 ;

    while (str [i] != 0 && i < NTAB (token_))
    {
	token_ [i] = str [i] ;
	i++ ;
    }

    toklen_ = i ;
}

/**
 * Constructor with an existing token
 *
 * This constructor copies an existing token into the current object
 *
 * @param val token value
 * @param len token length
 */

Token::Token (uint8_t *val, size_t len)
{
    if (len > 0 && len < NTAB (token_))
    {
	toklen_ = len ;
	memcpy (token_, val, len) ;
    }
    else toklen_ = 0 ;
}

/******************************************************************************
 * Operators
 */

bool Token::operator== (const Token &t)
{
    return toklen_ == t.toklen_ && memcmp (token_, t.token_, toklen_) == 0 ;
}
bool Token::operator!= (const Token &t)
{
    return toklen_ != t.toklen_ || memcmp (token_, t.token_, toklen_) != 0 ;
}

/******************************************************************************
 * Commodity functions
 */

void Token::reset (void)
{
    toklen_ = 0 ;
}

void Token::print (void)
{
    int i ;

    for (i = 0 ; i < toklen_ ; i++)
	DBG2 (token_ [i], HEX) ;
}
