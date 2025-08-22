#! /bin/sh

PROGRAM=me
CC=cc

CFLAGS="-O2 -g -Wall -Wextra -Wstrict-prototypes -Wno-unused-parameter"

SRCS=$(echo main.c buffer.c window.c line.c display.c input.c command.c \
	ebind.c file.c fileio.c search.c isearch.c posix.c ansi.c global.c \
	memory.c util.c | tr ' ' '\n')

OBJS=$(echo $SRCS | sed 's/\.c/.o/g')

for f in $SRCS; do
	echo "	CC	$f..."
	$CC $CFLAGS -c $f
done

echo "	Link	$PROGRAM"
$CC -o $PROGRAM $OBJS
