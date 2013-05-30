CXX = g++
L2ETH = -DUSE_PF_PACKET=1
#L2ETH = -DUSE_PCAP=1
CXXFLAGS = -g -DDEBUG -std=c++11 $(L2ETH) -Wall -W -pedantic -I ..
LDFLAGS = -L. -lsos -lpthread

LIBS = libsos.a
HDRS = coap.h sos.h l2.h l2eth.h option.h msg.h slave.h utils.h ../global.h
OBJS = l2eth.o l2.o option.o msg.o slave.o sos.o utils.o

all:	sendtest

sendtest: sendtest.o $(LIBS)
	c++ $(CXXFLAGS) -o sendtest sendtest.o $(LDFLAGS)

libsos.a: $(OBJS)
	ar r libsos.a $(OBJS)

*.o: $(HDRS)

clean:
	rm -f *.o sendtest libsos.a
