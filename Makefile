BIN_PATH = /usr/bin
PROGRAM = me
CC = cc

SRC = main.c buffer.c window.c line.c display.c input.c command.c ebind.c \
	file.c fileio.c search.c isearch.c posix.c ansi.c global.c memory.c \
	util.c

OBJ = main.o buffer.o window.o line.o display.o input.o command.o ebind.o \
	file.o fileio.o search.o isearch.o posix.o ansi.o global.o memory.o \
	util.o

CFLAGS = -O2 -g -Wall -Wextra -Wstrict-prototypes -Wno-unused-parameter

$(PROGRAM): $(OBJ)
	@echo "	LINK	$@"
	@$(CC) -o $@ $^

showkeys: showkeys.o posix.o global.o ansi.o
	@echo "	LINK	$@"
	@$(CC) -o $@ $^

clean:
	@rm -f $(PROGRAM) showkeys core *.o

install: $(PROGRAM)
	@echo "	$(BIN_PATH)/$(PROGRAM)"
	@cp me $(BIN_PATH)
	@strip $(BIN_PATH)/$(PROGRAM)
	@chmod 755 $(BIN_PATH)/$(PROGRAM)
	@echo

.c.o:
	@echo "	CC	$@"
	@$(CC) $(CFLAGS) -c $*.c

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
posix.o: posix.c estruct.h
ansi.o: ansi.c estruct.h edef.h
global.o: estruct.h edef.h
memory.o: estruct.h edef.h
util.o: util.c
