# @file Makefile
# @author Arslan Smajevic 12127678 <e12127678@student.tuwien.ac.at>
# @date 17.10.2023
#
# @brief Makefile for intmul

intmul: intmul.o
	gcc intmul.o -o intmul

intmul.o: intmul.c
	gcc -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g -c intmul.c -o intmul.o

clean: 
	rm intmul

.PHONY: all clean
all: intmul

release:
	tar -cvzf HW1A.tgz intmul.c Makefile

# CC = gcc
# DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
  
# CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
# OBJECTS = vigenere.o
# .PHONY: all clean

# all: intmul

# intmul: $(OBJECTS)
# 	$(CC) $(LDFLAGS) -o $@ $^

# %.o: %.c
# 	$(CC) $(CFLAGS) -c -o $@ $<

# vigenere.o: intmul.c 

# clean:
# 	rm -rf *.o intmul