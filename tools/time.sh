#!/bin/sh

TOOL=$HOME/devel/cunf/dev/src/main
T="/usr/bin/time"
F="command %C\nstatus %x\nelapsed %e\nuser %U"

if [ "$#" != "1" ]; then
	echo "Usage: time.sh NETFILE"
	exit 1
fi

OUT=`$T -f "$F" "$TOOL" "$1" 2>&1`

# cunf: Done, 4 events, 7 conditions, 7 histories.
# mole: Done, 4 events, 7 conditions.

H=`echo "$OUT" | grep "Done, " | sed 's/.*, \(.*\) histories.*/\1/'`
if [ "$H" = "" ]; then
	H=`echo "$OUT" | grep "Done, " | sed 's/.*, \(.*\) events.*/\1/'`
fi
S=`echo "$OUT" | grep "^status " | sed 's/status //'`
E=`echo "$OUT" | grep "^elapsed " | sed 's/elapsed //'`
U=`echo "$OUT" | grep "^user " | sed 's/user //'`

if [ "$H" = "" ]; then H="?"; fi
if [ "$S" = "" ]; then S="?"; fi
if [ "$E" = "" ]; then E="?"; fi
if [ "$U" = "" ]; then U="?"; fi

# stdout: status histories user elapsed net
# stderr: output

echo "$OUT" >&2
echo "stat\thist\tuser\telapsed\tnet"
echo "$S\t$H\t$U\t$E\t$1"
exit 0

