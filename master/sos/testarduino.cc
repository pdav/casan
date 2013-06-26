#include <iostream>
#include <cstring>
#include <vector>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "global.h"

#include "l2.h"
#include "l2eth.h"
#include "msg.h"
#include "resource.h"
#include "sos.h"

#define IFACE		"eth0"
#define ADDR		"00:01:02:03:04:05"		// arduino

#define	PATH_1		".well-known"
#define	PATH_2		"sos"

#define	HELLO_100	"hello=100"
#define ASSOCIATE	"assoc=30000"		// assoc=ttl
#define ASK_RESOURCES "/resources"

void connexion(void)
{
	sos::l2net *l ;
	sos::l2net_eth *le ;
	sos::l2addr_eth *sa ;		// slave address
	sos::slave s ;			// slave
	sos::slave sb ;			// pseudo-slave for broadcast
	sos::msg m1, m2, m3 ;

	// start new interface
	le = new sos::l2net_eth ;
	if (le->init (IFACE, ETHTYPE_SOS) == -1)
	{
		perror ("init") ;
		exit (1) ;
	}

	// from now on, use only generic l2net interface
	l = le ;

	// register new slave
	sa = new sos::l2addr_eth (ADDR) ;
	s.addr (sa) ;
	s.l2 (l) ;

	// pseudo-slave for broadcast address
	sb.addr (&sos::l2addr_eth_broadcast) ;
	sb.l2 (l) ;

	std::cout << IFACE << " initialized for " << ADDR << "\n" ;

	// sleep (1) ;

	sos::option uri_path1 (sos::option::MO_Uri_Path, (void *) PATH_1, sizeof PATH_1 - 1) ;
	sos::option uri_path2 (sos::option::MO_Uri_Path, (void *) PATH_2, sizeof PATH_2 - 1) ;

	// HELLO message
	sos::option opt_hello (sos::option::MO_Uri_Query, (void *) HELLO_100, sizeof HELLO_100 - 1) ;

	m1.type (sos::msg::MT_NON) ;
	m1.code (sos::msg::MC_POST) ;
	m1.peer (&sb) ;
	m1.id (1) ;
	m1.pushoption (uri_path1) ;
	m1.pushoption (uri_path2) ;
	m1.pushoption (opt_hello) ;
	m1.send () ;

	sleep (1) ;

	// ASSOCIATE message
	sos::option opt_assoc(sos::option::MO_Uri_Query, 
			(void *) ASSOCIATE, sizeof ASSOCIATE - 1);

	m2.type (sos::msg::MT_CON) ;
	m2.code (sos::msg::MC_POST);
	m2.peer (&s) ;
	m2.id (2) ;
	m2.pushoption (uri_path1) ;
	m2.pushoption (uri_path2) ;
	m2.pushoption(opt_assoc);
	m2.send () ;

	sleep(1);

	sos::option opt_ask_resources(sos::option::MO_Uri_Query, 
			(void *) ASK_RESOURCES, sizeof ASK_RESOURCES - 1);

	m3.type (sos::msg::MT_NON) ;
	m3.code (sos::msg::MC_GET);
	m3.peer (&s) ;
	m3.id (3) ;
	m3.pushoption(opt_ask_resources);
	m3.send () ;

	delete sa ;
	delete l ;
}

void request_hello(char *val)
{
	sos::l2net *l ;
	sos::l2net_eth *le ;
	sos::l2addr_eth *sa ;		// slave address
	sos::slave s ;			// slave
	sos::slave sb ;			// pseudo-slave for broadcast
	sos::msg m1;

	// start new interface
	le = new sos::l2net_eth ;
	if (le->init (IFACE, ETHTYPE_SOS) == -1)
	{
		perror ("init") ;
		exit (1) ;
	}

	// from now on, use only generic l2net interface
	l = le ;

	// register new slave
	sa = new sos::l2addr_eth (ADDR) ;
	s.addr (sa) ;
	s.l2 (l) ;

	// pseudo-slave for broadcast address
	sb.addr (&sos::l2addr_eth_broadcast) ;
	sb.l2 (l) ;

	std::cout << IFACE << " initialized for " << ADDR << "\n" ;

	sos::option uri_path1 (sos::option::MO_Uri_Path, (void *) PATH_1, sizeof PATH_1 - 1) ;
	sos::option uri_path2 (sos::option::MO_Uri_Path, (void *) PATH_2, sizeof PATH_2 - 1) ;

	char hello_msg[50];
	if(val)
		snprintf(hello_msg, 50, "hello=%s", val);
	else
		snprintf(hello_msg, 50, "hello=1000");


	// HELLO message
	sos::option opt_hello (sos::option::MO_Uri_Query, 
			(void *) hello_msg, 
			strlen(hello_msg)) ;

	m1.type (sos::msg::MT_NON) ;
	m1.code (sos::msg::MC_POST) ;
	m1.peer (&sb) ;
	m1.id (1) ;
	m1.pushoption (uri_path1) ;
	m1.pushoption (uri_path2) ;
	m1.pushoption (opt_hello) ;
	m1.send () ;

	delete sa ;
	delete l ;
}

void request_assoc(char *val)
{
	sos::l2net *l ;
	sos::l2net_eth *le ;
	sos::l2addr_eth *sa ;		// slave address
	sos::slave s ;			// slave
	sos::slave sb ;			// pseudo-slave for broadcast
	sos::msg m1;

	// start new interface
	le = new sos::l2net_eth ;
	if (le->init (IFACE, ETHTYPE_SOS) == -1)
	{
		perror ("init") ;
		exit (1) ;
	}

	// from now on, use only generic l2net interface
	l = le ;

	// register new slave
	sa = new sos::l2addr_eth (ADDR) ;
	s.addr (sa) ;
	s.l2 (l) ;

	std::cout << IFACE << " initialized for " << ADDR << "\n" ;

	sos::option uri_path1 (sos::option::MO_Uri_Path, (void *) PATH_1, sizeof PATH_1 - 1) ;
	sos::option uri_path2 (sos::option::MO_Uri_Path, (void *) PATH_2, sizeof PATH_2 - 1) ;

	char associate_msg[50];
	if(val)
		snprintf(associate_msg, 50, "assoc=%s", val);
	else
		snprintf(associate_msg, 50, "assoc=30000");


	// ASSOCIATE message
	sos::option opt_assoc(sos::option::MO_Uri_Query, 
			(void *) associate_msg, strlen(associate_msg));

	m1.type (sos::msg::MT_CON);
	m1.code (sos::msg::MC_POST);
	m1.peer (&s) ;
	m1.id(1);
	m1.pushoption (uri_path1) ;
	m1.pushoption (uri_path2) ;
	m1.pushoption(opt_assoc);
	m1.send() ;

	delete sa ;
	delete l ;
}

void requete_ressources(char * resource, char * val)
{
	sos::l2net *l ;
	sos::l2net_eth *le ;
	sos::l2addr_eth *sa ;		// slave address
	sos::slave s ;			// slave
	sos::slave sb ;			// pseudo-slave for broadcast
	sos::msg m ;

	// start new interface
	le = new sos::l2net_eth ;
	if (le->init (IFACE, ETHTYPE_SOS) == -1)
	{
		perror ("init") ;
		exit (1) ;
	}

	// from now on, use only generic l2net interface
	l = le ;

	// register new slave
	sa = new sos::l2addr_eth (ADDR) ;
	s.addr (sa) ;
	s.l2 (l) ;

	// pseudo-slave for broadcast address
	sb.addr (&sos::l2addr_eth_broadcast) ;
	sb.l2 (l) ;

	std::cout << IFACE << " initialized for " << ADDR << "\n" ;

	// sleep (1) ;

	sos::option opt_ask_resources(sos::option::MO_Uri_Query, 
			(void *) resource, strlen(resource));

	m.type (sos::msg::MT_NON);
	m.code (sos::msg::MC_GET);
	m.peer (&s);
	m.id (3);
	m.pushoption(opt_ask_resources);

	if(val != NULL)	
	{
		sos::option opt_resource_val(sos::option::MO_Uri_Query, 
				(void *) val, strlen(val));
		m.pushoption(opt_resource_val);
	}

	std::cout << "OPTCODE : " << opt_ask_resources.optcode() << std::endl;

	m.send () ;

	delete sa ;
	delete l ;
}

void display_help(void)
{
	std::cout << "Usage : ./testarduino [(hello|assoc|resource) [val]]" << std::endl;
}

int main (int argc, char **argv)
{
	switch (argc)
	{
		case 1 :
			connexion();
			break;

		case 2 :

			if(strcmp("assoc", argv[1]) == 0)
			{
				request_assoc(NULL);
			}
			else if(strcmp("hello", argv[1]) == 0)
			{
				request_hello(NULL);
			}
			else
			{
				requete_ressources(argv[1], NULL);
			}

			break;

		case 3 :

			if(strcmp("assoc", argv[1]) == 0)
			{
				request_assoc(argv[2]);
			}
			else if(strcmp("hello", argv[1]) == 0)
			{
				request_hello(argv[2]);
			}
			else
			{
				requete_ressources(argv[1], argv[2]);
			}

			break;

		default :
			display_help();
	}
	return 0;
}
