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
/tmp/dij03.ll_net
/tmp/dij04.ll_net
/tmp/dij05.ll_net

examples/plain/large/bds_1.sync.ll_net
examples/plain/large/byzagr4_1b.ll_net
examples/plain/large/ftp_1.sync.ll_net
examples/plain/small/dme2.ll_net
examples/plain/small/dme4.ll_net
examples/plain/med/dme6.ll_net
examples/plain/med/dme8.ll_net
examples/plain/med/dme10.ll_net
examples/plain/small/key_2.ll_net
examples/plain/med/key_3.ll_net
examples/plain/large/key_4.ll_net
examples/plain/small/elevator_2.ll_net
examples/plain/med/elevator_3.ll_net
examples/plain/large/elevator_4.ll_net
/tmp/plain-dij02.ll_net
/tmp/plain-dij03.ll_net
/tmp/plain-dij04.ll_net
/tmp/plain-dij05.ll_net
"

NETSDE="
#/tmp/dek20.ll_net
#/tmp/dek40.ll_net
#/tmp/dek60.ll_net

/tmp/plain-dek20.ll_net
/tmp/plain-dek40.ll_net
/tmp/plain-dek60.ll_net
"

RW="
examples/cont/large/rw_12.ll_net
examples/plain/large/rw_12.ll_net
"

for i in 02 03 04 05 06; do ./tools/mkdijkstra.py $i > /tmp/plain-dij$i.ll_net; done
for i in 20 40 60; do ./tools/mkdekker.py $i > /tmp/plain-dek$i.ll_net; done

echo >> TABLE
echo >> TABLE
date -R >> TABLE

for i in $RW; do
	n=`echo $i | sed 's/.ll_net$//'`
	echo $i
	echo cunf
	echo $i >> TABLE
	./src/main $i | grep 'event\|hist\|time' | grep -v 'At size' >> TABLE
	echo cna
	./tools/cna $n.unf.cuf -d -r sccred -n out
	echo minisat
	minisat out | grep 'CPU\|SATISF' >> TABLE
	cat TABLE
done

