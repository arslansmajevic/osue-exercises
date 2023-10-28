# @file Makefile
# @author Arslan Smajevic 12127678 <e12127678@student.tuwien.ac.at>
# @date 13.10.2023
#
# @brief Makefile for vigenere

# vigenere: vigenere.o
# 	gcc vigenere.o -o vigenere

# vigenere.o: vigenere.c
# 	gcc -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g -c vigenere.c -o vigenere.o

# clean: 
# 	rm vigenere

# .PHONY: all clean
# all: vigenere

# release:
# 	tar -cvzf HW1A.tgz vigenere.c Makefile

CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
  
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
OBJECTS = vigenere.o
.PHONY: all clean

all: vigenere

vigenere: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

vigenere.o: vigenere.c 

clean:
	rm -rf *.o vigenere