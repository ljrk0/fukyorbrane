CC=gcc
CFLAGS=-O2 -Wall -Werror -g

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
