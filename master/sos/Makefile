CXX = g++
L2ETH = -DUSE_PF_PACKET=1
#L2ETH = -DUSE_PCAP=1
CXXFLAGS = -g -DDEBUG -std=c++11 $(L2ETH) -Wall -W -pedantic -I ..

INCL = coap.h sos.h l2.h l2eth.h option.h msg.h slave.h utils.h ../global.h
OBJS = l2eth.o l2.o option.o msg.o slave.o sos.o utils.o

all:	sosd sendtest

sosd:	sosd.o libsos.a
	c++ $(CXXFLAGS) -o sosd sosd.o -L. -lsos -lpthread

sendtest: $(SOBJS)
	c++ $(CXXFLAGS) -o sendtest sendtest.o -L. -lsos -lpthread

libsos.a: $(LOBJS)
	ar r libsos.a $(LOBJS)

*.o: $(INCL)

clean:
	rm -f *.o sosd sendtest libsos.a
