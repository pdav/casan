CXXFLAGS = -g -std=c++11 -Wall -W -pedantic -I ../include

# common "library" files
LOBJS = l2.o l2eth.o engine.o msg.o slave.o utils.o

# object files for SOS daemon program
OBJS = sosd.o $(LOBJS)

# object files to test SOS packet sending
SOBJS = sendtest.o $(LOBJS)

INCL = sos.h l2.h l2eth.h engine.h msg.h slave.h utils.h ../include/defs.h

all:	sosd sendtest

sosd:	$(OBJS)
	c++ $(CXXFLAGS) -o sosd $(OBJS) -lpthread

sendtest: $(SOBJS)
	c++ $(CXXFLAGS) -o sendtest $(SOBJS) -lpthread

*.o: $(INCL)

clean:
	rm -f *.o sosd sendtest
