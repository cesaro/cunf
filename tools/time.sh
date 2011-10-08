#!/bin/bash

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

if [ "$#" -lt "1" ]; then
	echo "Usage: time.sh CMD"
	exit 1
fi

function output2table
{
	REPLY=x
	while true; do
		read
		K=`echo "$REPLY" | sed 's/\t.*//'`
		V=`echo "$REPLY" | sed 's/.*\t//'`
		if test "$K" == 'xj1234'; then
			echo "status$HDR"
			echo "$V$ROW"
			return
		fi
		HDR="$HDR	$K"
		ROW="$ROW	$V"
	done
}

{ sh -c "$*" 2>&1; echo "xj1234	$?"; } | output2table

exit 0

