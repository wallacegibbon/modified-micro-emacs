UNAME_S := $(shell sh -c 'uname -s 2>/dev/null || echo Unknown')
BIN_PATH = /usr/bin
PROGRAM = me
SHOWKEYS = showkeys
CC = cc

SRC = main.c buffer.c window.c line.c display.c input.c command.c ebind.c \
	file.c fileio.c search.c isearch.c spawn.c posix.c ansi.c global.c \
	memory.c util.c

OBJ = main.o buffer.o window.o line.o display.o input.o command.o ebind.o \
	file.o fileio.o search.o isearch.o spawn.o posix.o ansi.o global.o \
	memory.o util.o

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

$(SHOWKEYS): showkeys.o posix.o global.o ansi.o
	@echo "	LINK	$@"
	@$(CC) $(LDFLAGS) $(DEFINES) -o $@ $^ $(LIBS)

clean:
	@rm -f $(PROGRAM) $(SHOWKEYS) core *.o

install: $(PROGRAM)
	@echo "	$(BIN_PATH)/$(PROGRAM)"
	@cp me $(BIN_PATH)
	@strip $(BIN_PATH)/$(PROGRAM)
	@chmod 755 $(BIN_PATH)/$(PROGRAM)
	@echo

.c.o:
	@echo "	CC	$@"
	@$(CC) $(CFLAGS) $(DEFINES) -c $*.c

ebind.o: edef.h efunc.h estruct.h line.h
command.o: command.c estruct.h edef.h line.h
buffer.o: buffer.c estruct.h edef.h line.h
display.o: display.c estruct.h edef.h line.h
file.o: file.c estruct.h edef.h line.h
fileio.o: fileio.c estruct.h edef.h
input.o: input.c estruct.h edef.h
isearch.o: isearch.c estruct.h edef.h line.h
line.o: line.c estruct.h edef.h line.h
main.o: main.c estruct.h efunc.h edef.h line.h
posix.o: posix.c estruct.h
search.o: search.c estruct.h edef.h line.h
spawn.o: spawn.c estruct.h edef.h
window.o: window.c estruct.h edef.h line.h
global.o: estruct.h edef.h
memory.o: estruct.h edef.h
ansi.o: ansi.c estruct.h edef.h
util.o: util.c
