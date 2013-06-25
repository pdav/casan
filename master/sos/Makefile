CXX = g++
L2ETH = -DUSE_PF_PACKET=1
#L2ETH = -DUSE_PCAP=1
CXXFLAGS = -g -DDEBUG -std=c++11 $(L2ETH) -Wall -W -pedantic -I ..
LDFLAGS = -L. -lsos -lpthread

LIBS = libsos.a
HDRS = coap.h sos.h l2.h l2eth.h option.h msg.h slave.h resource.h waiter.h utils.h ../global.h
OBJS = l2eth.o l2.o option.o msg.o slave.o resource.o waiter.o sos.o utils.o

all:	libsos.a testsend testarduino

libsos.a: $(OBJS)
	ar r libsos.a $(OBJS)

testsend: testsend.o $(LIBS)
	c++ $(CXXFLAGS) -o testsend testsend.o $(LDFLAGS)

testarduino: testarduino.o $(LIBS)
	c++ $(CXXFLAGS) -o testarduino testarduino.o $(LDFLAGS)

*.o: $(HDRS)

clean:
	rm -f *.o libsos.a testsend testarduino
