# name     websh
# @file    Makefile
#
# @Author  Yannick Schwarenthorer, 1229026
# @date    3. 5. 2016
# @brief   executes a programm and transform its output for the web

CC = gcc
DEFS = -D_XOPEN_SOURCE=500 -D_BSD_SOURCE
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)

CFILES = websh.c fork_manager.c
HFILES = fork_manager.h
OBJECTFILES = websh.o fork_manager.o

all:websh

websh: $(OBJECTFILES) ; $(CC) $(LDFLAGS) -o $@ $^

%.o: %.c ; $(CC) $(CFLAGS) -c -o $@ $<

docs: $(OBJECTFILES) ; doxygen ../doc/Doxyfile

clean:
	rm -f $(OBJECTFILES)
	rm -f websh
	rm -rf html

.PHONY: clean all





