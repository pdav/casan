CXX = g++
L2ETH = -DUSE_PF_PACKET=1
#L2ETH = -DUSE_PCAP=1
CXXFLAGS = -g -DDEBUG -std=c++11 $(L2ETH) -Wall -Werror -W -pedantic -I ..
LDFLAGS = -L. -lsos -lpthread

LIBS = libsos.a
HDRS = coap.h sos.h l2.h l2-eth.h l2-154.h option.h msg.h slave.h resource.h waiter.h utils.h byte.h ../global.h
OBJS = l2-eth.o l2-154.o l2.o option.o msg.o slave.o resource.o waiter.o sos.o utils.o

all:	libsos.a testsend testarduino testxbee

libsos.a: $(OBJS)
	ar r libsos.a $(OBJS)

testsend: testsend.o $(LIBS)
	c++ $(CXXFLAGS) -o testsend testsend.o $(LDFLAGS)

testarduino: testarduino.o $(LIBS)
	c++ $(CXXFLAGS) -o testarduino testarduino.o $(LDFLAGS)

testxbee: testxbee.o $(LIBS)
	c++ $(CXXFLAGS) -o testxbee testxbee.o $(LDFLAGS)

test154: test154.o $(LIBS)
	c++ $(CXXFLAGS) -o test154 test154.o $(LDFLAGS)

*.o: $(HDRS)

clean:
	rm -f *.o libsos.a testsend testarduino testxbee
