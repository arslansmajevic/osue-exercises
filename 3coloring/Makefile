# @file Makefile
# @author Arslan Smajevic 12127678 <e12127678@student.tuwien.ac.at>
# @date 22.10.2023
#
# @brief Makefile for generator & supervisor

CC = gcc
# DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -DDEBUG
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L 
  
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
LDFLAGS = -pthread -lrt 
OBJECTS = supervisor.o generator.o
.PHONY: all clean

all: supervisor generator

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

supervisor: supervisor.o
	$(CC) -o supervisor supervisor.o $(LDFLAGS)

generator: generator.o
	$(CC) -o generator generator.o $(LDFLAGS)

supervisor.o: supervisor.c 
generator.o: generator.c

clean:
	rm -rf *.o supervisor generator

release:
	tar -cvzf 3coloringArslan.tar.gz supervisor.c generator.c Makefile