CC=gcc
CFLAGS=-std=c99
CFLAGS+=-Wall -Wpedantic -Wextra -Wshadow -Wconversion -Wno-sign-compare
CFLAGS+=-Werror

ifeq ($(DEBUG),1)
	CFLAGS+=-Og -g
else
	CFLAGS+=-O2
endif

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


FILES=$(wildcard fyb_src/*.fyb)
test:
	@$(foreach f, $(FILES), $(MAKE) F=$f test2;)

test2:
	@$(foreach f, $(FILES), ./fukyorbrane $f $F;)
