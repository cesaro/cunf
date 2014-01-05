#!/bin/sh

NETS="
examples/cont/large/bds_1.sync.ll_net
examples/cont/large/byzagr4_1b.ll_net
examples/cont/large/ftp_1.sync.ll_net
examples/cont/small/dme2.ll_net
examples/cont/small/dme4.ll_net
examples/cont/med/dme6.ll_net
examples/cont/med/dme8.ll_net
examples/cont/med/dme10.ll_net
examples/cont/small/key_2.ll_net
examples/cont/med/key_3.ll_net
examples/cont/large/key_4.ll_net
examples/cont/small/elevator_2.ll_net
examples/cont/med/elevator_3.ll_net
examples/cont/large/elevator_4.ll_net
/tmp/dij02.ll_net
/tmp/dij04.ll_net
/tmp/dij06.ll_net
"

#NETS="
#examples/cont/large/byzagr4_1b.ll_net
#"

for i in 02 04 06; do ./tools/mkdijkstra.py $i > /tmp/dij$i.ll_net; done

echo > TABLE

for i in $NETS; do
	n=`echo $i | sed 's/.ll_net$//'`
	echo $i
	echo cunf
	echo $i >> TABLE
	echo cna
	./src/main $i | grep 'event\|hist\|time' | grep -v 'At size' >> TABLE
	echo minisat
	minisat out | grep 'CPU\|SATISF' >> TABLE
	cat TABLE
done
