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
	grep -v 'test.nets.param.' | \
	grep -v 'test.nets.tiny.' | \
	grep -v 'test.nets.*.huge.'
}

if [ "$#" -eq "0" ]; then
	echo "Usage: nets.sh {all,test,time,tiny,small,med,large,huge,other,param,no-huge,mci}"
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
	"mci")		list | filter_mci ;;
	*)		echo "Invalid identifier '$1'" >&2
	esac
	shift
done

exit 0
