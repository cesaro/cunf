#!/bin/sh
NETS2="
examples/plain/small/do_od.ll_net
examples/plain/small/dme4.ll_net
examples/plain/small/cottbus_plate_5.ll_net
examples/plain/small/stack_full.ll_net
examples/plain/small/elevator_2.ll_net
examples/plain/small/sentest_75.fsa.ll_net
examples/plain/small/mutual.ll_net
examples/plain/small/sdl_example.ll_net
examples/plain/small/rrr10-1.sync.ll_net
examples/plain/small/ab_gesc.ll_net
examples/plain/small/sentest_25.fsa.ll_net
examples/plain/small/gas_station.ll_net
examples/plain/small/elevator_1.fsa.ll_net
examples/plain/small/sentest_50.fsa.ll_net
examples/plain/small/rw_1w1r.ll_net
examples/plain/small/eisenbahn.sync.ll_net
examples/plain/small/mmgt_1.fsa.ll_net
examples/plain/small/recursion.ll_net
examples/plain/small/peterson.ll_net
examples/plain/small/sdl_arq.ll_net
examples/plain/small/key_2.fsa.ll_net
examples/plain/small/byzagr4_0b.ll_net
examples/plain/small/dijkstra_2.sync.ll_net
examples/plain/small/mmgt_2.fsa.ll_net
examples/plain/small/byzagr4_0b.sync.ll_net
examples/plain/small/elevator.ll_net
examples/plain/small/rrr20-1.sync.ll_net
examples/plain/small/rw_1w1r.sync.ll_net
examples/plain/small/elevator_1.ll_net
examples/plain/small/reader_writer_2.ll_net
examples/plain/small/parrow.ll_net
examples/plain/small/sentest_100.fsa.ll_net
examples/plain/small/dme2.ll_net
examples/plain/small/furnace_1.fsa.ll_net
examples/plain/small/only_hl.ll_net
examples/plain/small/byzagr4_2a.ll_net
examples/plain/small/sdl_arq_deadlock.ll_net
examples/plain/small/rrr50-1.sync.ll_net
examples/plain/small/byzagr4_2a.sync.ll_net
examples/plain/small/rrr30-1.sync.ll_net
examples/plain/small/elevator_2.fsa.ll_net
examples/plain/small/dijkstra_2.ll_net
examples/plain/small/eisenbahn.ll_net
examples/plain/small/abp_1.fsa.ll_net
examples/plain/small/peterson_pfa.ll_net
examples/plain/small/cottbus_plate_5.sync.ll_net
examples/plain/small/key_2.ll_net
examples/plain/small/dme3.ll_net
examples/plain/med/q_1.fsa.ll_net
examples/plain/med/dme7.ll_net
examples/plain/med/elevator_3.ll_net
examples/plain/med/dme9.ll_net
examples/plain/med/dme10.ll_net
examples/plain/med/knuth_2.ll_net
examples/plain/med/elevator_3.dlmcs.sync.ll_net
examples/plain/med/dme6.ll_net
examples/plain/med/rw_1w2r.ll_net
examples/plain/med/rw_2w1r.sync.ll_net
examples/plain/med/dme8.ll_net
examples/plain/med/mmgt_3.fsa.ll_net
examples/plain/med/bruijn_2.sync.ll_net
examples/plain/med/rw_2w1r.ll_net
examples/plain/med/speed_1.fsa.ll_net
examples/plain/med/elevator_3.fsa.ll_net
examples/plain/med/key_3.fsa.ll_net
examples/plain/med/furnace_2.fsa.ll_net
examples/plain/med/key_3.ll_net
examples/plain/med/dme11.ll_net
examples/plain/med/q_1.ll_net
examples/plain/med/knuth_2.sync.ll_net
examples/plain/med/dme5.ll_net
examples/plain/med/bds_1.fsa.ll_net
examples/plain/med/bruijn_2.ll_net
examples/plain/large/elevator_4.old.ll_net
examples/plain/large/rw_1w3r.ll_net
examples/plain/large/elevator_4.dlmcs.sync.ll_net
examples/plain/large/furnace_3.fsa.ll_net
examples/plain/large/ftp_1.sync.ll_net
examples/plain/large/key_3.sync.ll_net
examples/plain/large/rw_12.sync.ll_net
examples/plain/large/rw_12.ll_net
examples/plain/large/elevator_4.ll_net
examples/plain/large/q_1.sync.ll_net
examples/plain/large/furnace_4.ll_net
examples/plain/large/bds_1.sync.ll_net
examples/plain/large/dpd_7.sync.ll_net
examples/plain/large/rw_1w3r.sync.ll_net
examples/plain/large/key_4.ll_net
examples/plain/large/furnace_3.ll_net
examples/plain/large/mmgt_4.fsa.ll_net
examples/plain/large/elevator_4.fsa.ll_net
examples/plain/large/ftp_1.fsa.ll_net
examples/plain/large/byzagr4_1b.ll_net
examples/plain/large/key_4.fsa.ll_net
examples/plain/huge/dme12.ll_net
examples/plain/huge/bw-nc-pc-nr-8.ll_net
examples/plain/huge/dartes_1.fsa.ll_net
"

NETS="
examples/plain/med/dme7.ll_net
examples/plain/med/dme9.ll_net
examples/plain/med/dme10.ll_net
examples/plain/med/dme6.ll_net
examples/plain/med/rw_2w1r.sync.ll_net
examples/plain/med/dme8.ll_net
examples/plain/med/mmgt_3.fsa.ll_net
examples/plain/med/key_3.ll_net
examples/plain/med/dme11.ll_net
examples/plain/med/q_1.ll_net
examples/plain/med/bds_1.fsa.ll_net
examples/plain/large/rw_1w3r.ll_net
examples/plain/large/furnace_3.fsa.ll_net
examples/plain/large/key_3.sync.ll_net
examples/plain/large/rw_12.ll_net
examples/plain/large/elevator_4.ll_net
examples/plain/large/furnace_4.ll_net
examples/plain/large/dpd_7.sync.ll_net
examples/plain/large/rw_1w3r.sync.ll_net
examples/plain/large/key_4.ll_net
examples/plain/large/furnace_3.ll_net
examples/plain/large/mmgt_4.fsa.ll_net
examples/plain/large/ftp_1.fsa.ll_net
examples/plain/large/byzagr4_1b.ll_net
"

echo >> TABLE
echo >> TABLE
date -R >> TABLE

for i in $NETS; do
	n=`echo $i | sed 's/.ll_net$//'`
	ls -lh $i
	echo >> TABLE
	echo $n >> TABLE
	#echo mole >> TABLE
	#(time mole $i) 2>&1 | grep 'user\|events\|VmPeak' >> TABLE
	#(time mole examples/plain/small/do_od.ll_net) 2>&1 | grep 'user\|events\|VmPeak' >> TABLE

	echo ngen3 >> TABLE
	(time ./ngen3/mole $i) 2>&1 | grep 'user\|events\|VmPeak' >> TABLE

	#echo punf >> TABLE
	#punf -s -t -n=0 -N=1 $i 2>&1 | grep folding >> TABLE
	#punf -s -t -n=0 -N=1 examples/plain/small/do_od.ll_net 2>&1 | grep folding >> TABLE
done

