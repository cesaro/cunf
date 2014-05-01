#!/bin/bash

TIMEOUT=3

rm -Rf "/tmp/cunftest.$$"
mkdir "/tmp/cunftest.$$"
mkdir "/tmp/cunftest.$$/bugs"
NET="/tmp/cunftest.$$/net"

echo ----------
echo $NET
echo ----------

while true; do
	# generate some net
	echo "==========================================================="
	N=`echo "1 + $RANDOM % 4" | bc`
	#./tools/mklife.py $N 2> $NET.ll_net 
	./tools/mklife-majority.pl $N > $NET.ll_net 

	# unfold it
	tools/trt.py t=cunf timeout=$TIMEOUT net=$NET.ll_net > $NET.unf.tr
	grep event $NET.unf.tr
	grep hist $NET.unf.tr
	if test "`grep stat $NET.unf.tr`" != "stat	0"; then
		echo "Unfolding killed; skiping"
		continue
	fi

	# generate reachable markings of the net
	test/net2dot $NET.ll_net > $NET.dot
	tools/rs.pl $NET.dot | sed '1d;$d' > $NET.r
	if test "`wc -l < $NET.r`" = "1" ; then
		echo "Only one reachable marking; skiping"
		continue
	fi

	# generate reachable markings of the unfolding
	test/cuf2dot $NET.unf.cuf
	tools/rs.pl $NET.unf.dot | sed '1d;$d' > $NET.unf.r

	# if reachable markings in net and unfolding differ, bug found!
	if ! diff $NET.r $NET.unf.r; then
		LINES=`wc -l < $NET.ll_net`
		EVENTS=`grep events $NET.unf.tr | sed 's/.*	//'`
		BUG=`mktemp /tmp/cunftest.$$/bugs/net.$EVENTS.$LINES.XXXXXX`
		cp $NET.ll_net $BUG
		mv $BUG $BUG.ll_net
		echo "========== Bug found! =========="
		echo "net $BUG.ll_net"
	fi
done
