BINS := wrmemcard

# BIN = $@

LCRD_OBJS = wrmemcard.o

all: $(BINS)

CFLAGS+=-Wall -pedantic -std=gnu99 -g -I/usr/include/PCSC
LFLAGS+=$(shell pkg-config --cflags libpcsclite)

wrmemcard: ${LCRD_OBJS}
	$(CC) $(CFLAGS) -o wrmemcard ${LCRD_OBJS} -lpcsclite

clean:
	$(RM) *.o $(BINS)
