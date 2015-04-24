#include <iostream>
#include <cstring>
#include <vector>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "global.h"

#include "l2.h"
#include "l2-eth.h"
#include "msg.h"
#include "resource.h"
#include "casan.h"

#define IFACE		"eth0"
#define ADDR		"00:01:02:03:04:05"		// arduino

#define	PATH_1		".well-known"
#define	PATH_2		"casan"

#define	HELLO_100	"hello=100"
#define ASSOCIATE	"ttl=30000"
#define ASK_RESOURCES "resources"

void connexion(void)
{
	casan::l2net *l ;
	casan::l2net_eth *le ;
	casan::l2addr_eth *sa ;		// slave address
	casan::slave s ;		// slave
	casan::slave sb ;		// pseudo-slave for broadcast
	casan::msg m1, m2, m3 ;

	// start new interface
	le = new casan::l2net_eth ;
	if (le->init (IFACE, ETHTYPE_CASAN) == -1)
	{
		perror ("init") ;
		exit (1) ;
	}

	// from now on, use only generic l2net interface
	l = le ;

	// register new slave
	sa = new casan::l2addr_eth (ADDR) ;
	s.addr (sa) ;
	s.l2 (l) ;

	// pseudo-slave for broadcast address
	sb.addr (&casan::l2addr_eth_broadcast) ;
	sb.l2 (l) ;

	std::cout << IFACE << " initialized for " << ADDR << "\n" ;

	// sleep (1) ;

	casan::option uri_path1 (casan::option::MO_Uri_Path, (void *) PATH_1, sizeof PATH_1 - 1) ;
	casan::option uri_path2 (casan::option::MO_Uri_Path, (void *) PATH_2, sizeof PATH_2 - 1) ;

	// HELLO message
	casan::option opt_hello (casan::option::MO_Uri_Query, (void *) HELLO_100, sizeof HELLO_100 - 1) ;

	m1.type (casan::msg::MT_NON) ;
	m1.code (casan::msg::MC_POST) ;
	m1.peer (&sb) ;
	m1.id (1) ;
	m1.pushoption (uri_path1) ;
	m1.pushoption (uri_path2) ;
	m1.pushoption (opt_hello) ;
	m1.send () ;

	sleep (1) ;

	// ASSOCIATE message
	casan::option opt_assoc(casan::option::MO_Uri_Query, 
			(void *) ASSOCIATE, sizeof ASSOCIATE - 1);

	m2.type (casan::msg::MT_CON) ;
	m2.code (casan::msg::MC_POST);
	m2.peer (&s) ;
	m2.id (2) ;
	m2.pushoption (uri_path1) ;
	m2.pushoption (uri_path2) ;
	m2.pushoption(opt_assoc);
	m2.send () ;

	sleep(1);

	casan::option opt_ask_resources(casan::option::MO_Uri_Path, 
			(void *) ASK_RESOURCES, sizeof ASK_RESOURCES - 1);

	m3.type (casan::msg::MT_NON) ;
	m3.code (casan::msg::MC_GET);
	m3.peer (&s) ;
	m3.id (3) ;
	m3.pushoption(opt_ask_resources);
	m3.send () ;

	delete sa ;
	delete l ;
}

void request_hello(char *val)
{
	casan::l2net *l ;
	casan::l2net_eth *le ;
	casan::l2addr_eth *sa ;		// slave address
	casan::slave s ;		// slave
	casan::slave sb ;		// pseudo-slave for broadcast
	casan::msg m1;

	// start new interface
	le = new casan::l2net_eth ;
	if (le->init (IFACE, ETHTYPE_CASAN) == -1)
	{
		perror ("init") ;
		exit (1) ;
	}

	// from now on, use only generic l2net interface
	l = le ;

	// register new slave
	sa = new casan::l2addr_eth (ADDR) ;
	s.addr (sa) ;
	s.l2 (l) ;

	// pseudo-slave for broadcast address
	sb.addr (&casan::l2addr_eth_broadcast) ;
	sb.l2 (l) ;

	std::cout << IFACE << " initialized for " << ADDR << "\n" ;

	casan::option uri_path1 (casan::option::MO_Uri_Path, (void *) PATH_1, sizeof PATH_1 - 1) ;
	casan::option uri_path2 (casan::option::MO_Uri_Path, (void *) PATH_2, sizeof PATH_2 - 1) ;

	char hello_msg[50];
	if(val)
		snprintf(hello_msg, 50, "hello=%s", val);
	else
		snprintf(hello_msg, 50, "hello=1000");


	// HELLO message
	casan::option opt_hello (casan::option::MO_Uri_Query, 
			(void *) hello_msg, 
			strlen(hello_msg)) ;

	m1.type (casan::msg::MT_NON) ;
	m1.code (casan::msg::MC_POST) ;
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
	casan::l2net *l ;
	casan::l2net_eth *le ;
	casan::l2addr_eth *sa ;		// slave address
	casan::slave s ;		// slave
	casan::slave sb ;		// pseudo-slave for broadcast
	casan::msg m1;

	// start new interface
	le = new casan::l2net_eth ;
	if (le->init (IFACE, ETHTYPE_CASAN) == -1)
	{
		perror ("init") ;
		exit (1) ;
	}

	// from now on, use only generic l2net interface
	l = le ;

	// register new slave
	sa = new casan::l2addr_eth (ADDR) ;
	s.addr (sa) ;
	s.l2 (l) ;

	std::cout << IFACE << " initialized for " << ADDR << "\n" ;

	casan::option uri_path1 (casan::option::MO_Uri_Path, (void *) PATH_1, sizeof PATH_1 - 1) ;
	casan::option uri_path2 (casan::option::MO_Uri_Path, (void *) PATH_2, sizeof PATH_2 - 1) ;

	char associate_msg[50];
	if(val)
		snprintf(associate_msg, 50, "ttl=%s", val);
	else
		snprintf(associate_msg, 50, "ttl=30000");


	// ASSOCIATE message
	casan::option opt_assoc(casan::option::MO_Uri_Query, 
			(void *) associate_msg, strlen(associate_msg));

	m1.type (casan::msg::MT_CON);
	m1.code (casan::msg::MC_POST);
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
	casan::l2net *l ;
	casan::l2net_eth *le ;
	casan::l2addr_eth *sa ;		// slave address
	casan::slave s ;		// slave
	casan::slave sb ;		// pseudo-slave for broadcast
	casan::msg m ;

	// start new interface
	le = new casan::l2net_eth ;
	if (le->init (IFACE, ETHTYPE_CASAN) == -1)
	{
		perror ("init") ;
		exit (1) ;
	}

	// from now on, use only generic l2net interface
	l = le ;

	// register new slave
	sa = new casan::l2addr_eth (ADDR) ;
	s.addr (sa) ;
	s.l2 (l) ;

	// pseudo-slave for broadcast address
	sb.addr (&casan::l2addr_eth_broadcast) ;
	sb.l2 (l) ;

	std::cout << IFACE << " initialized for " << ADDR << "\n" ;

	// sleep (1) ;

	casan::option opt_ask_resources(casan::option::MO_Uri_Path, 
			(void *) resource, strlen(resource));

	m.type (casan::msg::MT_CON);
	m.code (casan::msg::MC_GET);
	m.peer (&s);
	m.id (3);
	m.pushoption(opt_ask_resources);

	if(val != NULL)	
	{
		char valeur[50];
		snprintf(valeur, 50, "val=%s", val);
		m.payload((void *) valeur, strlen(valeur));
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
