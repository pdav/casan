#include "message.h"

Message::Message ()
{
    type_ = 0 ;
    code_ = 0 ;
    id_ = 0 ;
    toklen_ = 0 ;
    token_ = NULL ;
    paylen_ = 0 ;
    optlen_ = 0 ;
    payload_ = NULL ;
    optlist_ = NULL ;
    curopt_ = NULL ;
    curopt_initialized_ = false ;
}

Message::Message (Message & c) 
{
    id_ = c.id_ ;
    type_ = c.type_ ;
    code_ = c.code_ ;
    toklen_ = c.toklen_ ;
    token_ = c.get_token_copy () ;
    paylen_ = c.paylen_ ;
    payload_ = c.get_payload_copy () ;
}

Message & Message::operator= (const Message &m) 
{
    if (this != &m)
    {
	id_ = m.id_ ;
	this->set_token (m.toklen_, m.token_) ;
	this->set_payload (m.paylen_, m.payload_) ;
    }
    return *this ;
}

bool Message::operator== (const Message &m)
{
    return this->id_ == m.id_ ;
}

Message::~Message () 
{
    if (token_ != NULL)
	delete token_ ;
    if (payload_ != NULL)
	delete payload_ ;
    free_options () ;
}

// TODO : finish this function
void Message::reset (void)
{
    if (token_ != NULL)
	delete token_ ;
    if (payload_ != NULL)
	delete payload_ ;
    free_options () ;
    type_ = 0 ;
    code_ = 0 ;
    id_ = 0 ;
    toklen_ = 0 ;
    token_ = NULL ;
    paylen_ = 0 ;
    payload_ = NULL ;
    optlen_ = 0 ;
    optlist_ = NULL ;
    curopt_ = NULL ;
    curopt_initialized_ = false ;
    reset_get_option () ;
}

uint8_t Message::get_type (void) 
{
    return type_ ;
}

uint8_t Message::get_code (void) 
{
    return code_ ;
}

int Message::get_id (void) 
{
    return id_ ;
}

uint8_t Message::get_toklen (void) 
{
    return toklen_ ;
}

uint8_t * Message::get_token (void)
{
    return token_ ;
}

uint8_t * Message::get_token_copy (void) 
{
    uint8_t * copy = (uint8_t*) malloc (toklen_) ;
    memcpy (copy, token_, toklen_) ;
    return copy ;
}

uint8_t Message::get_paylen (void) 
{
    return paylen_ ;
}

uint8_t * Message::get_payload (void) 
{
    return payload_ ;
}

uint8_t * Message::get_payload_copy (void) 
{
    uint8_t * copy = (uint8_t*) malloc (paylen_ +1) ;
    memcpy (copy, payload_, paylen_) ;
    copy[paylen_] = '\0' ;
    return copy ;
}

option * Message::pop_option (void) 
{
    option *r = NULL ;
    if (optlist_ != NULL)
    {
	optlist *next ;

	r = optlist_->o ;
	next = optlist_->next ;
	delete optlist_ ;
	optlist_ = next ;
    }
    return r ;
}

void Message::set_type (uint8_t t) 
{
    type_ = t ;
}

void Message::set_code (uint8_t c) 
{
    code_ = c ;
}

void Message::set_id (int id) 
{
    id_ = id ;
}

void Message::set_token (uint8_t token_length, uint8_t *token)
{
    toklen_ = token_length ;
    token_ = (uint8_t*) malloc (token_length) ;
    memcpy (token_, token, token_length) ;
}

void Message::set_payload (uint8_t payload_length, uint8_t * payload) 
{
    paylen_ = payload_length ;
    if (payload_ != NULL)
	free (payload_) ;
    payload_ = (uint8_t*) malloc (payload_length) ;
    memcpy (payload_, payload, payload_length) ;
}

// push option in the sorted list of options
void Message::push_option (option &o) 
{
    optlist *newo, *prev, *cur ;

    newo = (optlist *) malloc (sizeof *newo) ;
    newo->o = new option (o) ;
    prev = NULL ;
    cur = optlist_ ;

    while (cur != NULL && *(newo->o) >= *(cur->o))
    {
	prev = cur ;
	cur = cur->next ;
    }

    newo->next = cur ;
    if (prev == NULL)
	optlist_ = newo ;
    else
	prev->next = newo ;
}

void Message::free_options (void) 
{
    while (optlist_ != NULL)
	delete pop_option () ;
}

void Message::reset_get_option (void) 
{
    curopt_initialized_ = false ;
}

option *Message::get_option (void) 
{
    option *o ;
    if (! curopt_initialized_)
    {
	curopt_ = optlist_ ;
	curopt_initialized_ = true ;
    }
    if (curopt_ == NULL)
    {
	o = NULL ;
	curopt_initialized_ = false ;
    }
    else
    {
	o = curopt_->o ;
	curopt_ = curopt_->next ;
    }
    return o ;
}

void Message::print (void) 
{
    Serial.print (F ("\033[36mmsg\033[00m\tid=")) ;
    Serial.print (get_id ()) ;
    Serial.print (F (", type=")) ;
    switch (get_type ()) {
	case COAP_TYPE_CON : Serial.print ("CON") ; break ;
	case COAP_TYPE_NON : Serial.print ("NON") ; break ;
	case COAP_TYPE_ACK : Serial.print ("ACK") ; break ;
	case COAP_TYPE_RST : Serial.print ("RST") ; break ;
	default : Serial.print ("\033[31mERROR\033[00m") ;
    }
    Serial.print (F (", code=")) ;
    Serial.print (get_code () >> 5, HEX) ;
    Serial.print (".") ;
    Serial.print (get_code () & 0x1f, HEX) ;
    Serial.print (F (", toklen=")) ;
    Serial.print (get_toklen ()) ;

    if (get_toklen () > 0) {
	Serial.print (F (", token=")) ;
	uint8_t *token = get_token () ;
	for (int i = 0 ; i < get_toklen () ; i++)
	    Serial.print (token [i], HEX) ;
	Serial.println () ;
    }

    Serial.print (F (", paylen=")) ;
    Serial.print (get_paylen ()) ;
    if (get_paylen () > 0) {
	Serial.print (F ("  payload=")) ;
	uint8_t *pcopy = get_payload_copy () ;
	Serial.print ((char *) pcopy) ;
	free (pcopy) ;
    }
    Serial.println () ;

    for (option *o = get_option () ; o != NULL ; o = get_option ()) {
	o->print () ;
    }
}
