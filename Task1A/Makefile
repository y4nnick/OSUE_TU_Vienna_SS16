# name     stegit
# @file    Makefile
#
# @Author  Yannick Schwarenthorer, 1229026
# @date    11. April 2016
# @brief   Simple chiffre module
# @detail  This program encrypts plain text into encrypted text and reverse. The output could be written to a file

CC = gcc
DEFS = -D_XOPEN_SOURCE=500 -D_BSD_SOURCE
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
OBJECTFILES = stegit.o

all:stegit

stegit: $(OBJECTFILES) ; $(CC) $(LDFLAGS) -o $@ $^

%.o: %.c ; $(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTFILES)
	rm -f stegit