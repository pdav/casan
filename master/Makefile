CXX = g++
L2ETH = -DUSE_PF_PACKET=1
#L2ETH = -DUSE_PCAP=1
INCLUDES = -Isos -Ihttp -I.
CXXFLAGS = -g -DDEBUG -std=c++11 $(L2ETH) -Wall -W -pedantic $(INCLUDES)
LDFLAGS = -Lsos -lsos -Lhttp -lhttp -lpthread

LIBS = sos/libsos.a http/libhttp.a
HDRS = conf.h global.h request_handler.hpp
OBJS = sosd.o conf.o request_handler.o

all:	libs $(LIBS) sosd

libs:
	cd sos ; make
	cd http ; make

sosd:	$(OBJS) $(LIBS)
	c++ $(CXXFLAGS) -o sosd $(OBJS) $(LDFLAGS)

*.o: $(HDRS)

clean:
	cd sos ; make clean
	cd http ; make clean
	rm -f *.o sosd
