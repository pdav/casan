CXX = g++				# Linux with g++ >= 4.7
# CXX = c++				# FreeBSD with Clang >= 3.4

L2ETH = -DUSE_PF_PACKET=1		# Linux
# L2ETH = -DUSE_PCAP=1			# FreeBSD (not available)
# L2ETH =				# no Ethernet Support

ASIOINC =				# Linux
# ASIOINC = -I/usr/local/include	# FreeBSD

INCLUDES = -Icasan -Ihttp -I. $(ASIOINC)
CXXFLAGS = -g -std=c++11 $(L2ETH) -Wall -Werror -W -pedantic $(INCLUDES)
LDFLAGS = -Lcasan -lcasan -Lhttp -lhttp -lpthread

LIBS = casan/libcasan.a http/libhttp.a
HDRS = conf.h global.h master.h request_handler.hpp
OBJS = casand.o conf.o master.o request_handler.o

all:	libs $(LIBS) casand

libs:
	cd casan ; make CXX="$(CXX)" L2ETH="$(L2ETH)" libcasan.a
	cd http ; make CXX="$(CXX)" ASIOINC="$(ASIOINC)" libhttp.a

casand:	$(OBJS) $(LIBS)
	c++ $(CXXFLAGS) -o casand $(OBJS) $(LDFLAGS)

html:
	doxygen

pdf:
	doxygen
	cd doc/latex ; make

*.o: $(HDRS)

clean:
	cd casan ; make clean
	cd http ; make clean
	rm -f *.o casand
	rm -rf doc
