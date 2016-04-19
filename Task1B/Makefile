# name     Mastermind
# @file    Makefile
#
# @Author  Yannick Schwarenthorer, 1229026
# @date    11. April 2016
# @brief   Mastermind game

CC = gcc
DEFS = -D_XOPEN_SOURCE=500 -D_BSD_SOURCE
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
OBJECTFILES_SERVER = server.o
OBJECTFILES_CLIENT = client.o

all:server client

server: $(OBJECTFILES_SERVER) ; $(CC) $(LDFLAGS) -o $@ $^

client: $(OBJECTFILES_CLIENT) ; $(CC) $(LDFLAGS) -o $@ $^

%.o: %.c ; $(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTFILES_SERVER)
	rm -f $(OBJECTFILES_CLIENT)
	rm -f server
	rm -f client