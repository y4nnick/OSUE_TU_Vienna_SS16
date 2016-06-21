# Author: Yannick Schwarenthorer
# Date: 11.06.2016
# name     Hangman
# @file    Makefile
# @Author  Yannick Schwarenthorer, 1229026
# @date    3. 5. 2016

CC = gcc
DEFS = -D_XOPEN_SOURCE=500 -D_BSD_SOURCE
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
LDFLAGS = -lrt -pthread

CFILES_SERVER = hangman-server.c fileRead.c
HFILES_SERVER = fileRead.h hangman-common.h
OBJECTFILES_SERVER = hangman-server.o fileRead.o

CFILES_CLIENT = hangman-client.c
HFILES_SERVER = hangman-common.h
OBJECTFILES_CLIENT = hangman-client.o

all: hangman-server hangman-client

hangman-server: $(OBJECTFILES_SERVER)
	$(CC) -o $@ $^ $(LDFLAGS)

hangman-client: $(OBJECTFILES_CLIENT)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

docs: $(OBJECTFILES)
	doxygen ../doc/Doxyfile

clean:
	rm -f $(OBJECTFILES_CLIENT)
	rm -f $(OBJECTFILES_SERVER)
	rm -f hangman-server
	rm -f hangman-client
	rm -rf html

.PHONY: clean all

