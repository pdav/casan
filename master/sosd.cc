#include <iostream>

#include <unistd.h>

#include "conf.h"
#include "sos.h"

int main (int argc, char *argv [])
{
    conf cf ;
    char *conffile ;

    if (argc != 2)
    {
	std::cerr << "usage: " << argv [0] << " conf-file\n" ;
	exit (1) ;
    }
    conffile = argv [1] ;

    if (! cf.init (conffile))
    {
	std::cout << "Read error for " << conffile << "\n" ;
	std::cout << "Abort\n" ;
	exit (1) ;
    }

    std::cout << cf ;

    cf.start () ;

    sleep (10000) ;

    exit (0) ;
}
