ifeq ($(WINBUILD),1)
CC=x86_64-w64-mingw32-gcc
EXESUFF=.exe
endif

ifndef CC
CC=gcc
endif

$(info "$$(CC) is [$(CC)]")

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

EXEFILES=$(addsuffix $(EXESUFF), fukyorbrane)

all: $(EXEFILES)

$(EXEFILES): $(OFILES)
	$(CC) $(CFLAGS) $(OFILES) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OFILES) $(EXEFILES)


FILES=$(wildcard fyb_src/*.fyb)
test:
	@$(foreach f, $(FILES), $(MAKE) F=$f test2;)

test2:
	@$(foreach f, $(FILES), ./fukyorbrane $f $F;)
