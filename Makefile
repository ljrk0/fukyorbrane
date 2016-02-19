CC=gcc
CFLAGS=-std=c99
CFLAGS+=-O2 -Wall -Wpedantic -Wextra -Wshadow -Wconversion -Wno-sign-compare -g
#CFLAGS+=-Werror

CFILES=fukyorbrane.c

OFILES=fukyorbrane.o

.SUFFIXES: .c .o

all: fukyorbrane

fukyorbrane: $(OFILES)
	$(CC) $(CFLAGS) $(OFILES) -o fukyorbrane

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OFILES) fukyorbrane
