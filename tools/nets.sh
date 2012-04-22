#!/bin/bash

function list
{
	find test/nets/ -name '*.xml' -or -name '*.ll_net' | \
	sed 's/.xml$/.ll_net/' | sort -u
}

function filter_test
{
	grep 'test.nets.*small' | \
	grep -v byzagr4 | \
	grep -v rrr | \
	grep -v eisenbahn | \
	#grep -v dme4 | \
	#grep -v parrow | \
	grep -v cottbus_plate_5
}

function filter_time
{
	grep -v 'test.nets.other.' | \
	grep -v 'test.nets.param.' | \
	grep -v 'test.nets.tiny.' | \
	grep -v 'test.nets.*.huge.'
}

function filter_mci
{
	grep -v 'test.nets.cont.' | \
	grep -v 'test.nets.param.[^bc].*net' | \
	grep -v 'test.nets.param.boolc.cont.' | \
	grep -v 'test.nets.param.cellular.cont.' | \
	grep -v 'test.nets.tiny.' | \
	grep -v 'test.nets.*.huge.'
}

function tcs_paper_corbett
{
	find test/nets/{plain,cont}/{large/bds_1.sync,large/byzagr4_1b,large/dpd_7.sync,large/elevator_4.old,large/ftp_1.sync,large/furnace_4,large/key_4,large/mmgt_4.fsa,large/q_1.sync,large/rw_12.sync,large/rw_1w3r,med/bruijn_2.sync,med/dme11,med/knuth_2,med/rw_2w1r,med/speed_1.fsa}.ll_net
}

function tcs_paper_example3
{
	find test/nets/{plain,pr,cont}/param/r/r*.ll_net
}

function tcs_and
{
	find test/nets/{plain,pr,cont}/param/andnet/and*.ll_net | \
	grep -v 'nets.plain.*and1[5-9]' | \
	grep -v 'nets.plain.*and[2-9]'
}

function concur12
{
	echo "test/nets/$1/med/bds_1.fsa.ll_net"
	echo "test/nets/$1/large/rw_12.ll_net"
	echo "test/nets/$1/med/dme7.ll_net"
	echo "test/nets/$1/large/furnace_3.fsa.ll_net"
	echo "test/nets/$1/large/bds_1.sync.ll_net"
	echo "test/nets/$1/med/rw_2w1r.ll_net"
	echo "test/nets/$1/med/dme8.ll_net"
	echo "test/nets/$1/large/rw_12.sync.ll_net"
	echo "test/nets/$1/large/dpd_7.sync.ll_net"
	echo "test/nets/$1/large/furnace_3.ll_net"
	echo "test/nets/$1/large/rw_1w3r.ll_net"
	echo "test/nets/$1/med/dme9.ll_net"
	echo "test/nets/$1/med/dme10.ll_net"
	echo "test/nets/$1/large/ftp_1.fsa.ll_net"
	echo "test/nets/$1/large/byzagr4_1b.ll_net"
	echo "test/nets/$1/med/dme11.ll_net"
	echo "test/nets/$1/large/ftp_1.sync.ll_net"
	echo "test/nets/$1/large/furnace_4.ll_net"
	echo "test/nets/$1/large/q_1.sync.ll_net"
	echo "test/nets/$1/med/key_3.ll_net"
	echo "test/nets/$1/large/elevator_4.fsa.ll_net"
	echo "test/nets/$1/large/key_3.sync.ll_net"
	echo "test/nets/$1/large/mmgt_4.fsa.ll_net"
	echo "test/nets/$1/large/key_4.ll_net"
	echo "test/nets/$1/large/elevator_4.old.ll_net"
}

function concur12_table2
{
	echo "test/nets/cont/large/byzagr4_1b.ll_net"
	echo "test/nets/cont/med/dme9.ll_net"
	echo "test/nets/cont/med/dme10.ll_net"
	echo "test/nets/cont/med/rw_2w1r.ll_net"
	echo "test/nets/cont/large/rw_1w3r.ll_net"
}

if [ "$#" -eq "0" ]; then
	echo "Usage: nets.sh ID (see the code :)"
	exit 1
fi

while [ -n "$*" ] ; do
	case "$1" in
	"all")		list ;;
	"test")		list | filter_test ;;
	"time")		list | filter_time ;;
	"tiny")		list | grep 'test.nets.tiny.' ;;
	"small")	list | grep 'test.nets.*.small.' ;;
	"med")		list | grep 'test.nets.*.med.' ;;
	"large")	list | grep 'test.nets.*.large.' ;;
	"huge")		list | grep 'test.nets.*.huge.' ;;
	"other")	list | grep 'test.nets.other.' ;;
	"param")	list | grep 'test.nets.param.' ;;
	"no-huge")	list | grep -v 'test.nets.*.huge.' ;;
	"tcs-paper-corbett")	tcs_paper_corbett ;;
	"tcs-paper-ex3")	tcs_paper_example3 ;;
	"tcs-andnets")	tcs_and ;;
	"concur12_plain")	concur12 "plain";;
	"concur12_cont")	concur12 "cont";;
	"concur12_table2")	concur12_table2;;
	"mci")		list | filter_mci ;;
	"cont-no-huge")	list | grep 'test.nets.cont.' | grep -v 'test.nets.*.huge.';;
	*)		echo "Invalid identifier '$1'" >&2
	esac
	shift
done

exit 0
