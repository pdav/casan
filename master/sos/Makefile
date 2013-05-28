CXX = g++
L2ETH = -DUSE_PF_PACKET=1
#L2ETH = -DUSE_PCAP=1
CXXFLAGS = -g -DDEBUG -std=c++11 $(L2ETH) -Wall -W -pedantic -I ..

# common "library" files
LOBJS = l2.o l2eth.o sos.o option.o msg.o slave.o utils.o

# object files for SOS daemon program
OBJS = sosd.o $(LOBJS)

# object files to test SOS packet sending
SOBJS = sendtest.o $(LOBJS)

INCL = coap.h sos.h l2.h l2eth.h option.h msg.h slave.h utils.h ../global.h

all:	sosd sendtest

sosd:	$(OBJS)
	c++ $(CXXFLAGS) -o sosd $(OBJS) -lpthread

sendtest: $(SOBJS)
	c++ $(CXXFLAGS) -o sendtest $(SOBJS) -lpthread

*.o: $(INCL)

clean:
	rm -f *.o sosd sendtest
