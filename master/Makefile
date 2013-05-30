CXX = g++
L2ETH = -DUSE_PF_PACKET=1
#L2ETH = -DUSE_PCAP=1
INCLUDES = -Isos
CXXFLAGS = -g -DDEBUG -std=c++11 $(L2ETH) -Wall -W -pedantic $(INCLUDES)
LDFLAGS = -Lsos -lsos -lpthread

LIBS = sos/libsos.a
HDRS = conf.h global.h
OBJS = sosd.o conf.o

all:	libs sosd

libs:
	cd sos ; make

sosd:	$(OBJS) $(LIBS)
	c++ $(CXXFLAGS) -o sosd $(OBJS) $(LDFLAGS)

*.o: $(HDRS)

clean:
	cd sos ; make clean
	rm -f *.o sosd
