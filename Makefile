PROGRAM = me
BIN = /usr/bin
CC = cc
STRIP = strip --remove-section=.eh_frame --remove-section=.eh_frame_hdr

CFLAGS = -O2 -g -Wall -Wextra -Wstrict-prototypes -Wno-unused-parameter

OBJS = main.o buffer.o window.o line.o display.o input.o command.o ebind.o \
	file.o fileio.o search.o isearch.o global.o memory.o util.o \
	ansi.o posix.o unix.o

$(PROGRAM): $(OBJS)
	$(CC) -o $@ $(OBJS)

SHOWKEYS_OBJS = showkeys.o global.o ansi.o posix.o unix.o

showkeys: $(SHOWKEYS_OBJS)
	$(CC) -o $@ $(SHOWKEYS_OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(PROGRAM) showkeys core *.o

install: $(PROGRAM)
	cp $(PROGRAM) $(BIN)
	$(STRIP) $(BIN)/$(PROGRAM)
	chmod 755 $(BIN)/$(PROGRAM)

main.o: main.c estruct.h efunc.h edef.h line.h
buffer.o: buffer.c estruct.h edef.h line.h
window.o: window.c estruct.h edef.h line.h
line.o: line.c estruct.h edef.h line.h
display.o: display.c estruct.h edef.h line.h
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
