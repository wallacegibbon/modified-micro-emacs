#! /bin/sh

PROGRAM=showkey
CC=cc

CFLAGS="-O2 -g -Wall -Wextra -Wstrict-prototypes -Wno-unused-parameter"

SRCS=$(echo showkey.c global.c ansi.c posix.c unix.c | tr ' ' '\n')

OBJS=$(echo $SRCS | sed 's/\.c/.o/g')

for f in $SRCS; do
	echo $CC $CFLAGS -c $f
	$CC $CFLAGS -c $f
done

echo $CC -o $PROGRAM $OBJS
$CC -o $PROGRAM $OBJS
