# Convenienve for setting a windows build
ifeq ($(WIN),1)
EXESUFF=.exe
CC=x86_64-w64-mingw32-cc
endif

# Suffix for executable (eg .exe when building for Windows)
EXESUFF ?=
CC ?= gcc

#$(info "$$(CC) is [$(CC)]")

CFLAGS ?= -std=c99
CFLAGS += -Wall -Wpedantic -Wextra -Wshadow -Wconversion
CFLAGS += -Werror

ifeq ($(DEBUG),1)
CFLAGS += -Og -g
else
CFLAGS += -O2
endif

# Use implicit rules only for these suffixes
.SUFFIXES:
.SUFFIXES: .c .o

OFILES=fukyorbrane.o
EXEFILES=$(addsuffix $(EXESUFF), fukyorbrane)

all: $(EXEFILES)

$(EXEFILES): $(OFILES)
	$(CC) $(LDFLAGS) $(OFILES) -o $@

clean:
	$(RM) $(OFILES) $(EXEFILES)


FILES=$(wildcard fyb_src/*.fyb)
test: $(EXEFILES)
	@$(foreach f, $(FILES), $(MAKE) F=$f _test;)

_test:
	@$(foreach f, $(FILES), ./$(EXEFILES) $f $F;)
