#
# Makefile for the diganal tool
# C 2017 - 2023 Rainer Müller
#
# This Makefile is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

BINS := diganal dat2wav
		
.PHONY:	all check install clean

CFLAGS+=-g -O2 -MD -fstack-protector-strong -Wformat -Werror=format-security -Wall -Wextra -Wmaybe-uninitialized -std=gnu99 -pedantic-errors
 
# modifications for special environment
-include /etc/os-release
ifneq ($(findstring OpenWrt, $(NAME)), )
	DESTDIR=/usr/bin
else
	DESTDIR=/usr/local/bin
	ifneq ($(findstring raspbian, $(ID)), )
		BINS += fang
	endif
endif

all:	$(BINS)

install: all
	mkdir -p $(DESTDIR)
	cp -u -p $(BINS) $(DESTDIR)

check:
	cppcheck --enable=all . 2> cppcheck.txt

diganal: diganal.o analyse.o
	$(CC) $(CFLAGS) -o diganal diganal.o analyse.o 

dat2wav: dat2wav.o
	$(CC) $(CFLAGS) -o dat2wav dat2wav.o

fang: pifang.o
	$(CC) $(CFLAGS) -o fang pifang.o

clean:
	$(RM) *.o *.d $(BINS)

-include *.d
