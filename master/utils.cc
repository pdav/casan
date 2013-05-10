#include <chrono>
#include <cstdlib>

#include "sos.h"
#include "utils.h"

/******************************************************************************
 * Some utility functions
 */

/*
 * returns an integer peuso-random value in [0..n[
 */

int random_value (int n)
{
    return std::rand () % n ;
}


/*
 * returns a random delay between [0...maxmilli] milliseconds
 */

duration_t random_timeout (int maxmilli)
{
    int delay ;

    delay = random_value (maxmilli + 1) ;
    return duration_t (delay) ;
}

