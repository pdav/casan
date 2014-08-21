CXX = g++
ASIOINC =
CXXFLAGS = -g -DDEBUG -std=c++11 -Wall -W -pedantic -I.. $(ASIOINC)
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

all:	libhttp.a

libhttp.a: $(OBJS)
	ar r libhttp.a $(OBJS)

*.o: $(HDRS)

clean:
	rm -f *.o libhttp.a tags

tags:
	ctags --extra=+qf *.[ch]*
