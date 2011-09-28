#!/bin/bash

function list 
{
	find test/nets/tiny/*.xml
	find test/nets/plain/small/*.ll_net
	find test/nets/cont/small/*.ll_net
	find test/nets/pr/small/*.ll_net
}

function filterout
{
	sed 's/.xml$/.ll_net/' | \
	grep -v byzagr4 | \
	grep -v rrr | \
	grep -v eisenbahn | \
	#grep -v dme4 | \
	#grep -v parrow | \
	grep -v cottbus_plate_5
}

list | filterout
