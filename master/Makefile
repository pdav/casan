CFLAGS = -g

CXXFLAGS = -g -std=gnu++11

OBJS = main.o l2.o l2eth.o engine.o req.o

x:	$(OBJS)
	c++ -std=gnu++11 -g -o x $(OBJS) -lpthread

#eth: eth.o net.o
#	cc -g -o e eth.o net.o

#x: x.o l2ethernet.o
#	c++ -g -o x x.o l2ethernet.o
#
#t: test.cpp
#	c++ -g -o t test.cpp



# c++ -std=gnu++11 -g -o t t.cc -lpthread

clean:
	rm -f *.o x
