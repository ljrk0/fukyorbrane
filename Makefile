# Convenience for setting a windows build
ifeq ($(WIN),1)
EXESUFF ?= .exe
OBJPREF ?= win_
ifndef ($(CC))
CC = x86_64-w64-mingw32-cc
endif
endif

# Suffix for executable (eg .exe when building for Windows)
# Prefix for objects incl. executable (eg win_ for Windows)
EXESUFF ?=
OBJPREF ?=
CC ?= gcc

CFLAGS ?= -std=c99
CFLAGS += -Wall -Wpedantic -Wextra -Wshadow -Wconversion
CFLAGS += -Werror

ifeq ($(DEBUG),1)
CFLAGS += -Og -g
CPPFLAGS += -DDEBUG
else
CPPFLAGS += -DNDEBUG
CFLAGS += -O2
endif

# Use implicit rules only for these suffixes
.SUFFIXES:
.SUFFIXES: .c .o .obj

APPNAME=fukyorbrane
OFILES=$(addsuffix .o, $(addprefix $(OBJPREF), $(APPNAME)))
EXEFILES=$(addsuffix $(EXESUFF), $(addprefix $(OBJPREF), $(APPNAME)))

$(info == CONFIGURATION ==)
$(info Target(s)   [$(EXEFILES)])
$(info $$(CC)       [$(CC)])
$(info $$(CPPFLAGS) [$(CPPFLAGS)])
$(info $$(CFLAGS)   [$(CFLAGS)])

all: $(EXEFILES)

# Hack.
# TODO: Use the same (implicit) rule as for .o files but just name them
# 	differently. For now we just override both rules so we at least
# 	compile them both the same way...
$(OBJPREF)%.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

$(EXEFILES): $(OFILES)
	$(CC) $(LDFLAGS) $(OFILES) -o $@

clean:
	$(RM) $(OFILES) $(EXEFILES)


FILES=$(wildcard fyb_src/*.fyb)
test: $(EXEFILES)
	@$(foreach f, $(FILES), $(MAKE) F=$f _test;)

_test:
	@$(foreach f, $(FILES), ./$(EXEFILES) $f $F;)
