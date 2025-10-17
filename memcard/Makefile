# This file is free software; C 2025 Rainer Müller

BINS := rdmemcard wrmemcard
CFLAGS+=-Wall -pedantic -std=gnu99 -g -I/usr/include/PCSC

DESTDIR=/usr/local/bin

CC = gcc


.PHONY:	all check install clean

all: $(BINS)

rdmemcard: rdmemcard.o
	$(CC) $(CFLAGS) -o rdmemcard rdmemcard.o -lpcsclite

wrmemcard: wrmemcard.o
	$(CC) $(CFLAGS) -o wrmemcard wrmemcard.o -lpcsclite

clean:
	$(RM) *.o $(BINS)

check:
	cppcheck --enable=all --suppress=missingIncludeSystem .

install: all
	cp -p $(BINS) $(DESTDIR)
