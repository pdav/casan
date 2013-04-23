CXXFLAGS = -g -std=c++11

OBJS = main.o l2.o l2eth.o engine.o req.o slave.o

INCL = engine.h l2eth.h l2.h msg.h req.h slave.h sos.h

x:	$(OBJS)
	c++ $(CXXFLAGS) -o x $(OBJS) -lpthread

*.o: $(INCL)

clean:
	rm -f *.o x
