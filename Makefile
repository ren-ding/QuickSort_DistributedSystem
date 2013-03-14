# makefile for comp2310 assignment 2, 2012 
# written by Peter Strazdins RSCS ANU, 09/12                                   
# version 1.1 16/10/12

# usage: to compile and link the bsystem, use 
#	make 
# To remove all generatec files, use
#	make clean


.SUFFIXES:

OBJS=quicklib.o distquicklib.o
EXES=quicksort quicksort-nomc

CC=gcc
CFLAGS=-O2 -Wall
INCLUDEFLAGS=
ARCHFLAGS=

MCFLAGS=-lmcheck
LDFLAGS=-lpthread -lm

default: $(EXES)

quicksort-nomc: quicksort.o $(OBJS) 
	$(CC) $(CFLAGS) -o quicksort-nomc quicksort.o $(OBJS) $(LDFLAGS) 

%: %.o $(OBJS)
	$(CC) $(CFLAGS) -o $* $*.o $(OBJS) $(LDFLAGS) $(MCFLAGS)


%.o: %.c  
	$(CC) $(CFLAGS) $(ARCHFLAGS) $(INCLUDEFLAGS) -c $*.c

clean:
	rm -f $(EXES) *.o
