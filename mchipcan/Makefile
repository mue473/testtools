#
# This file is free software; C 2024 - 2025 Rainer Müller
#

BINS := mchipcan
OBJ = mchipcan.o parspi.o statistics.o
CFLAGS+=-Wall -MD -pedantic -std=gnu99 -g

DESTDIR=/usr/local/bin

CC = gcc


.PHONY:	all check install clean

all: $(BINS)

$(BINS): $(OBJ)
	$(CC) $(CFLAGS) -o $(BINS) $(OBJ)

clean:
	rm -f *.o *.d $(BINS)

check:
	cppcheck --enable=all --suppress=missingIncludeSystem .

install: all
	chown root:root $(BINS)
	chmod 4711 $(BINS)
	cp -p $(BINS) $(DESTDIR)

-include *.d
