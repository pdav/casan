CXX = g++
CXXFLAGS = -g -DDEBUG -std=c++11 -Wall -W -pedantic -I..
LDFLAGS = -L. -lhttp -lpthread

HDRS = \
	connection.hpp \
	header.hpp \
	io_service_pool.hpp \
	mime_types.hpp \
	reply.hpp \
	request.hpp \
	request_parser.hpp \
	server.hpp

OBJS = \
	connection.o \
	io_service_pool.o \
	mime_types.o \
	reply.o \
	request_parser.o \
	server.o \

#	request_handler.hpp \
#	request_handler.o \


all:	libhttp.a	# testhttp

libhttp.a: $(OBJS)
	ar r libhttp.a $(OBJS)

testhttp: posix_main.o libhttp.a
	c++ $(CXXFLAGS) -o testhttpd posix_main.o $(LDFLAGS)

*.o: $(HDRS)

clean:
	rm -f *.o testhttp libhttp.a tags

tags:
	ctags --extra=+qf *.[ch]*
