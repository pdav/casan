CXXFLAGS = -g -std=gnu++11

OBJS = main.o l2.o l2eth.o engine.o req.o slave.o

x:	$(OBJS)
	c++ $(CXXFLAGS) -o x $(OBJS) -lpthread

clean:
	rm -f *.o x
