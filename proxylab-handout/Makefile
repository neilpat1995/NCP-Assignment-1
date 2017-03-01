#Sample Makefile. You can make changes to this file according to your need
# The executable must be named proxy

#CHANGE: LDFLAGS changed from -lpthread to -pthread (necessary for the "make" command to execute properly).

CC = gcc
CFLAGS = -Wall -g 
LDFLAGS = -pthread

OBJS = proxy.o csapp.o

all: proxy

proxy: $(OBJS)

csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c
	$(CC) $(CFLAGS) -c proxy.c
clean:
	rm -f *~ *.o proxy 

