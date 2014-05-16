CXX = g++				# Linux with g++ >= 4.7
# CXX = c++				# FreeBSD with Clang >= 3.4

L2ETH = -DUSE_PF_PACKET=1		# Linux
# L2ETH = -DUSE_PCAP=1			# FreeBSD (not available)
# L2ETH =				# no Ethernet Support

ASIOINC =				# Linux
# ASIOINC = -I/usr/local/include	# FreeBSD

INCLUDES = -Isos -Ihttp -I. $(ASIOINC)
CXXFLAGS = -g -std=c++11 $(L2ETH) -Wall -Werror -W -pedantic $(INCLUDES)
LDFLAGS = -Lsos -lsos -Lhttp -lhttp -lpthread

LIBS = sos/libsos.a http/libhttp.a
HDRS = conf.h global.h master.h request_handler.hpp
OBJS = sosd.o conf.o master.o request_handler.o

all:	libs $(LIBS) sosd

libs:
	cd sos ; make CXX="$(CXX)" L2ETH="$(L2ETH)" libsos.a
	cd http ; make CXX="$(CXX)" ASIOINC="$(ASIOINC)" libhttp.a

sosd:	$(OBJS) $(LIBS)
	c++ $(CXXFLAGS) -o sosd $(OBJS) $(LDFLAGS)

html:
	doxygen

pdf:
	doxygen
	cd doc/latex ; make

*.o: $(HDRS)

clean:
	cd sos ; make clean
	cd http ; make clean
	rm -f *.o sosd
	rm -rf doc
