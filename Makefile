UNAME_S := $(shell sh -c 'uname -s 2>/dev/null || echo Unknown')
BINDIR = /usr/bin
PROGRAM = me
SHOWKEYS = showkeys
CC = gcc

SRC = main.c buffer.c window.c line.c display.c basic.c random.c \
	file.c fileio.c input.c search.c isearch.c lock.c region.c spawn.c \
	ansi.c posix.c ebind.c globals.c memory.c util.c

OBJ = main.o buffer.o window.o line.o display.o basic.o random.o \
	file.o fileio.o input.o search.o isearch.o lock.o region.o spawn.o \
	ansi.o posix.o ebind.o globals.o memory.o util.o

WARNINGS = -Wall -Wextra -Wstrict-prototypes -Wno-unused-parameter

#CFLAGS = -O0 $(WARNINGS) -g
CFLAGS = -O2 $(WARNINGS) -g

ifeq ($(UNAME_S), Linux)
DEFINES += -DPOSIX -DUSG -D_XOPEN_SOURCE=600 -D_GNU_SOURCE
endif

ifeq ($(UNAME_S), FreeBSD)
DEFINES += -DPOSIX -DSYSV -D_XOPEN_SOURCE=600 -D_BSD_SOURCE -D_SVID_SOURCE \
	-D_FREEBSD_C_SOURCE
endif

ifeq ($(UNAME_S), Darwin)
DEFINES += -DPOSIX -DSYSV -D_XOPEN_SOURCE=600 -D_BSD_SOURCE -D_SVID_SOURCE \
	-D_DARWIN_C_SOURCE
endif

$(PROGRAM): $(OBJ)
	@echo "	LINK	$@"
	@$(CC) $(LDFLAGS) $(DEFINES) -o $@ $^ $(LIBS)

$(SHOWKEYS): showkeys.o posix.o globals.o ansi.o
	@echo "	LINK	$@"
	@$(CC) $(LDFLAGS) $(DEFINES) -o $@ $^ $(LIBS)

clean:
	@rm -f $(PROGRAM) $(SHOWKEYS) core *.o

install: $(PROGRAM)
	@echo "	$(BINDIR)/$(PROGRAM)"
	@cp me $(BINDIR)
	@strip $(BINDIR)/$(PROGRAM)
	@chmod 755 $(BINDIR)/$(PROGRAM)
	@echo

.c.o:
	@echo "	CC	$@"
	@$(CC) $(CFLAGS) $(DEFINES) -c $*.c

ebind.o: edef.h efunc.h estruct.h line.h
basic.o: basic.c estruct.h edef.h line.h
buffer.o: buffer.c estruct.h edef.h line.h
display.o: display.c estruct.h edef.h line.h
file.o: file.c estruct.h edef.h line.h
fileio.o: fileio.c estruct.h edef.h
input.o: input.c estruct.h edef.h
isearch.o: isearch.c estruct.h edef.h line.h
line.o: line.c estruct.h edef.h line.h
lock.o: lock.c estruct.h edef.h
main.o: main.c estruct.h efunc.h edef.h line.h
posix.o: posix.c estruct.h
random.o: random.c estruct.h edef.h line.h
region.o: region.c estruct.h edef.h line.h
search.o: search.c estruct.h edef.h line.h
spawn.o: spawn.c estruct.h edef.h
window.o: window.c estruct.h edef.h line.h
globals.o: estruct.h edef.h
memory.o: estruct.h edef.h
ansi.o: ansi.c estruct.h edef.h
util.o: util.c
