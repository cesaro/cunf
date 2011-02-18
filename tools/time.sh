#!/bin/sh

# Copyright (C) 2010, 2011  Cesar Rodriguez <cesar.rodriguez@lsv.ens-cachan.fr>
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <http://www.gnu.org/licenses/>.

T="/usr/bin/time"
F="command %C\nstatus %x\nelapsed %e\nuser %U"

if [ "$#" != "2" ]; then
	echo "Usage: time.sh UNFOLDER NET"
	exit 1
fi

OUT=`$T -f "$F" "$1" "$2" 2>&1`

# cunf: Done, 4 events, 7 conditions, 7 histories.
# mole: Done, 4 events, 7 conditions.

H=`echo "$OUT" | grep "Done, " | grep " histories" | sed 's/.*, \(.*\) histories.*/\1/'`
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
/bin/echo -e "stat\thist\tuser\telapsed\tnet"
/bin/echo -e "$S\t$H\t$U\t$E\t$2"
exit 0

