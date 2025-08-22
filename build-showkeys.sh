#! /bin/sh

PROGRAM=showkeys
CC=cc

CFLAGS="-O2 -g -Wall -Wextra -Wstrict-prototypes -Wno-unused-parameter"

SRCS=$(echo showkeys.c posix.c global.c ansi.c | tr ' ' '\n')

OBJS=$(echo $SRCS | sed 's/\.c/.o/g')

for f in $SRCS; do
	echo "	CC	$f..."
	$CC $CFLAGS -c $f
done

echo "	Link	$PROGRAM"
$CC -o $PROGRAM $OBJS
