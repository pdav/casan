CXXFLAGS = -g -std=c++11 -Wall -W -pedantic -I ../include

OBJS = main.o l2.o l2eth.o engine.o msg.o slave.o utils.o

INCL = sos.h l2.h l2eth.h engine.h msg.h slave.h utils.h

x:	$(OBJS)
	c++ $(CXXFLAGS) -o x $(OBJS) -lpthread

*.o: $(INCL)

clean:
	rm -f *.o x
