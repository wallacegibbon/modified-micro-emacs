## Keep this Makefile ANSI-compliant.
PROGRAM = me
BIN = /usr/bin
CC = cc
STRIP = strip --remove-section=.eh_frame --remove-section=.eh_frame_hdr

CFLAGS = -O2 -g -Wall -Wextra -Wstrict-prototypes -Wno-unused-parameter

PLATFORM_OBJS = posix.o unix.o

OBJS = display.o window.o line.o buffer.o input.o command.o ebind.o global.o \
	file.o fileio.o search.o isearch.o memory.o util.o ansi.o \
	$(PLATFORM_OBJS)

$(PROGRAM): main.o $(OBJS)
	$(CC) -o $@ main.o $(OBJS)

showkey: showkey.o $(OBJS)
	$(CC) -o $@ showkey.o $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(PROGRAM) showkey core *.o

install: $(PROGRAM)
	cp $(PROGRAM) $(BIN)
	$(STRIP) $(BIN)/$(PROGRAM)
	chmod 755 $(BIN)/$(PROGRAM)

main.o: main.c estruct.h efunc.h edef.h line.h
display.o: display.c estruct.h edef.h line.h
window.o: window.c estruct.h edef.h line.h
line.o: line.c estruct.h edef.h line.h
buffer.o: buffer.c estruct.h edef.h line.h
input.o: input.c estruct.h edef.h
command.o: command.c estruct.h edef.h line.h
ebind.o: edef.h efunc.h estruct.h line.h
file.o: file.c estruct.h edef.h line.h
fileio.o: fileio.c estruct.h edef.h
search.o: search.c estruct.h edef.h line.h
isearch.o: isearch.c estruct.h edef.h line.h
global.o: estruct.h edef.h
memory.o: estruct.h edef.h
util.o: util.c
ansi.o: ansi.c estruct.h edef.h
posix.o: posix.c estruct.h
unix.o: unix.c
