CC = g++
CFLAGS = -O2
OBJS = hostname.o rconvlog.o misc.o

rconvlog: $(OBJS)
	$(CC) -o $@ $(OBJS)

hostname.o: hostname.cpp hostname.h rconvlog.h misc.h
	$(CC) $(CFLAGS) -c $*.cpp

rconvlog.o: rconvlog.cpp hostname.h rconvlog.h misc.h
	$(CC) $(CFLAGS) -c $*.cpp

misc.o: misc.cpp misc.h
	$(CC) $(CFLAGS) -c $*.cpp

clean:
	rm $(OBJS)
	rm rconvlog
