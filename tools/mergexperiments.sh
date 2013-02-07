#!/bin/sh

PLAIN="
test/nets/plain/small/do_od
test/nets/plain/small/dme4
test/nets/plain/small/cottbus_plate_5
test/nets/plain/small/stack_full
test/nets/plain/small/elevator_2
test/nets/plain/small/sentest_75.fsa
test/nets/plain/small/mutual
test/nets/plain/small/sdl_example
test/nets/plain/small/rrr10-1.sync
test/nets/plain/small/ab_gesc
test/nets/plain/small/sentest_25.fsa
test/nets/plain/small/gas_station
test/nets/plain/small/elevator_1.fsa
test/nets/plain/small/sentest_50.fsa
test/nets/plain/small/rw_1w1r
test/nets/plain/small/eisenbahn.sync
test/nets/plain/small/mmgt_1.fsa
test/nets/plain/small/recursion
test/nets/plain/small/peterson
test/nets/plain/small/sdl_arq
test/nets/plain/small/key_2.fsa
test/nets/plain/small/byzagr4_0b
test/nets/plain/small/dijkstra_2.sync
test/nets/plain/small/mmgt_2.fsa
test/nets/plain/small/byzagr4_0b.sync
test/nets/plain/small/elevator
test/nets/plain/small/rrr20-1.sync
test/nets/plain/small/rw_1w1r.sync
test/nets/plain/small/elevator_1
test/nets/plain/small/reader_writer_2
test/nets/plain/small/parrow
test/nets/plain/small/sentest_100.fsa
test/nets/plain/small/dme2
test/nets/plain/small/furnace_1.fsa
test/nets/plain/small/only_hl
test/nets/plain/small/byzagr4_2a
test/nets/plain/small/sdl_arq_deadlock
test/nets/plain/small/rrr50-1.sync
test/nets/plain/small/byzagr4_2a.sync
test/nets/plain/small/rrr30-1.sync
test/nets/plain/small/elevator_2.fsa
test/nets/plain/small/dijkstra_2
test/nets/plain/small/eisenbahn
test/nets/plain/small/abp_1.fsa
test/nets/plain/small/peterson_pfa
test/nets/plain/small/cottbus_plate_5.sync
test/nets/plain/small/key_2
test/nets/plain/small/dme3
test/nets/plain/med/q_1.fsa
test/nets/plain/med/dme7
test/nets/plain/med/elevator_3
test/nets/plain/med/dme9
test/nets/plain/med/dme10
test/nets/plain/med/knuth_2
test/nets/plain/med/elevator_3.dlmcs.sync
test/nets/plain/med/dme6
test/nets/plain/med/rw_1w2r
test/nets/plain/med/rw_2w1r.sync
test/nets/plain/med/dme8
test/nets/plain/med/mmgt_3.fsa
test/nets/plain/med/bruijn_2.sync
test/nets/plain/med/rw_2w1r
test/nets/plain/med/speed_1.fsa
test/nets/plain/med/elevator_3.fsa
test/nets/plain/med/key_3.fsa
test/nets/plain/med/furnace_2.fsa
test/nets/plain/med/key_3
test/nets/plain/med/dme11
test/nets/plain/med/q_1
test/nets/plain/med/knuth_2.sync
test/nets/plain/med/dme5
test/nets/plain/med/bds_1.fsa
test/nets/plain/med/bruijn_2
test/nets/plain/large/elevator_4.old
test/nets/plain/large/rw_1w3r
test/nets/plain/large/elevator_4.dlmcs.sync
test/nets/plain/large/furnace_3.fsa
test/nets/plain/large/ftp_1.sync
test/nets/plain/large/key_3.sync
test/nets/plain/large/rw_12.sync
test/nets/plain/large/rw_12
test/nets/plain/large/elevator_4
test/nets/plain/large/q_1.sync
test/nets/plain/large/furnace_4
test/nets/plain/large/bds_1.sync
test/nets/plain/large/dpd_7.sync
test/nets/plain/large/rw_1w3r.sync
tttttest/nets/plain/large/key_4
test/nets/plain/large/furnace_3
test/nets/plain/large/mmgt_4.fsa
test/nets/plain/large/elevator_4.fsa
test/nets/plain/large/ftp_1.fsa
test/nets/plain/large/byzagr4_1b
test/nets/plain/large/key_4.fsa
"

PR="
test/nets/pr/small/do_od
test/nets/pr/small/dme4
test/nets/pr/small/cottbus_plate_5
test/nets/pr/small/stack_full
test/nets/pr/small/elevator_2
test/nets/pr/small/sentest_75.fsa
test/nets/pr/small/mutual
test/nets/pr/small/sdl_example
test/nets/pr/small/rrr10-1.sync
test/nets/pr/small/ab_gesc
test/nets/pr/small/sentest_25.fsa
test/nets/pr/small/gas_station
test/nets/pr/small/elevator_1.fsa
test/nets/pr/small/sentest_50.fsa
test/nets/pr/small/rw_1w1r
test/nets/pr/small/eisenbahn.sync
test/nets/pr/small/mmgt_1.fsa
test/nets/pr/small/recursion
test/nets/pr/small/peterson
test/nets/pr/small/sdl_arq
test/nets/pr/small/key_2.fsa
test/nets/pr/small/byzagr4_0b
test/nets/pr/small/dijkstra_2.sync
test/nets/pr/small/mmgt_2.fsa
test/nets/pr/small/byzagr4_0b.sync
test/nets/pr/small/elevator
test/nets/pr/small/rrr20-1.sync
test/nets/pr/small/rw_1w1r.sync
test/nets/pr/small/elevator_1
test/nets/pr/small/reader_writer_2
test/nets/pr/small/parrow
test/nets/pr/small/sentest_100.fsa
test/nets/pr/small/dme2
test/nets/pr/small/furnace_1.fsa
test/nets/pr/small/only_hl
test/nets/pr/small/byzagr4_2a
test/nets/pr/small/sdl_arq_deadlock
test/nets/pr/small/rrr50-1.sync
test/nets/pr/small/byzagr4_2a.sync
test/nets/pr/small/rrr30-1.sync
test/nets/pr/small/elevator_2.fsa
test/nets/pr/small/dijkstra_2
test/nets/pr/small/eisenbahn
test/nets/pr/small/abp_1.fsa
test/nets/pr/small/peterson_pfa
test/nets/pr/small/cottbus_plate_5.sync
test/nets/pr/small/key_2
test/nets/pr/small/dme3
test/nets/pr/med/q_1.fsa
test/nets/pr/med/dme7
test/nets/pr/med/elevator_3
test/nets/pr/med/dme9
test/nets/pr/med/dme10
test/nets/pr/med/knuth_2
test/nets/pr/med/elevator_3.dlmcs.sync
test/nets/pr/med/dme6
test/nets/pr/med/rw_1w2r
test/nets/pr/med/rw_2w1r.sync
test/nets/pr/med/dme8
test/nets/pr/med/mmgt_3.fsa
test/nets/pr/med/bruijn_2.sync
test/nets/pr/med/rw_2w1r
test/nets/pr/med/speed_1.fsa
test/nets/pr/med/elevator_3.fsa
test/nets/pr/med/key_3.fsa
test/nets/pr/med/furnace_2.fsa
test/nets/pr/med/key_3
test/nets/pr/med/dme11
test/nets/pr/med/q_1
test/nets/pr/med/knuth_2.sync
test/nets/pr/med/dme5
test/nets/pr/med/bds_1.fsa
test/nets/pr/med/bruijn_2
test/nets/pr/large/elevator_4.old
test/nets/pr/large/rw_1w3r
test/nets/pr/large/elevator_4.dlmcs.sync
test/nets/pr/large/furnace_3.fsa
test/nets/pr/large/ftp_1.sync
test/nets/pr/large/key_3.sync
test/nets/pr/large/rw_12.sync
test/nets/pr/large/rw_12
test/nets/pr/large/elevator_4
test/nets/pr/large/q_1.sync
test/nets/pr/large/furnace_4
test/nets/pr/large/bds_1.sync
test/nets/pr/large/dpd_7.sync
test/nets/pr/large/rw_1w3r.sync
test/nets/pr/large/key_4
test/nets/pr/large/furnace_3
test/nets/pr/large/mmgt_4.fsa
test/nets/pr/large/elevator_4.fsa
test/nets/pr/large/ftp_1.fsa
test/nets/pr/large/byzagr4_1b
test/nets/pr/large/key_4.fsa
"

CONT="
test/nets/cont/small/do_od
test/nets/cont/small/dme4
test/nets/cont/small/cottbus_plate_5
test/nets/cont/small/stack_full
test/nets/cont/small/elevator_2
test/nets/cont/small/sentest_75.fsa
test/nets/cont/small/mutual
test/nets/cont/small/sdl_example
test/nets/cont/small/rrr10-1.sync
test/nets/cont/small/ab_gesc
test/nets/cont/small/sentest_25.fsa
test/nets/cont/small/gas_station
test/nets/cont/small/elevator_1.fsa
test/nets/cont/small/sentest_50.fsa
test/nets/cont/small/rw_1w1r
test/nets/cont/small/eisenbahn.sync
test/nets/cont/small/mmgt_1.fsa
test/nets/cont/small/recursion
test/nets/cont/small/peterson
test/nets/cont/small/sdl_arq
test/nets/cont/small/key_2.fsa
test/nets/cont/small/byzagr4_0b
test/nets/cont/small/dijkstra_2.sync
test/nets/cont/small/mmgt_2.fsa
test/nets/cont/small/byzagr4_0b.sync
test/nets/cont/small/elevator
test/nets/cont/small/rrr20-1.sync
test/nets/cont/small/rw_1w1r.sync
test/nets/cont/small/elevator_1
test/nets/cont/small/reader_writer_2
test/nets/cont/small/parrow
test/nets/cont/small/sentest_100.fsa
test/nets/cont/small/dme2
test/nets/cont/small/furnace_1.fsa
test/nets/cont/small/only_hl
test/nets/cont/small/byzagr4_2a
test/nets/cont/small/sdl_arq_deadlock
test/nets/cont/small/rrr50-1.sync
test/nets/cont/small/byzagr4_2a.sync
test/nets/cont/small/rrr30-1.sync
test/nets/cont/small/elevator_2.fsa
test/nets/cont/small/dijkstra_2
test/nets/cont/small/eisenbahn
test/nets/cont/small/abp_1.fsa
test/nets/cont/small/peterson_pfa
test/nets/cont/small/cottbus_plate_5.sync
test/nets/cont/small/key_2
test/nets/cont/small/dme3
test/nets/cont/med/q_1.fsa
test/nets/cont/med/dme7
test/nets/cont/med/elevator_3
test/nets/cont/med/dme9
test/nets/cont/med/dme10
test/nets/cont/med/knuth_2
test/nets/cont/med/elevator_3.dlmcs.sync
test/nets/cont/med/dme6
test/nets/cont/med/rw_1w2r
test/nets/cont/med/rw_2w1r.sync
test/nets/cont/med/dme8
test/nets/cont/med/mmgt_3.fsa
test/nets/cont/med/bruijn_2.sync
test/nets/cont/med/rw_2w1r
test/nets/cont/med/speed_1.fsa
test/nets/cont/med/elevator_3.fsa
test/nets/cont/med/key_3.fsa
test/nets/cont/med/furnace_2.fsa
test/nets/cont/med/key_3
test/nets/cont/med/dme11
test/nets/cont/med/q_1
test/nets/cont/med/knuth_2.sync
test/nets/cont/med/dme5
test/nets/cont/med/bds_1.fsa
test/nets/cont/med/bruijn_2
test/nets/cont/large/elevator_4.old
test/nets/cont/large/rw_1w3r
test/nets/cont/large/elevator_4.dlmcs.sync
test/nets/cont/large/furnace_3.fsa
test/nets/cont/large/ftp_1.sync
test/nets/cont/large/key_3.sync
test/nets/cont/large/rw_12.sync
test/nets/cont/large/rw_12
test/nets/cont/large/elevator_4
test/nets/cont/large/q_1.sync
test/nets/cont/large/furnace_4
test/nets/cont/large/bds_1.sync
test/nets/cont/large/dpd_7.sync
test/nets/cont/large/rw_1w3r.sync
test/nets/cont/large/key_4
test/nets/cont/large/furnace_3
test/nets/cont/large/mmgt_4.fsa
test/nets/cont/large/elevator_4.fsa
test/nets/cont/large/ftp_1.fsa
test/nets/cont/large/byzagr4_1b
test/nets/cont/large/key_4.fsa
"

#echo >> out
#date -R >> out
#
##for n in $PLAIN; do
##	echo $n
##	punf -n=200000 -N=1 -s -t -@4 -# $n.ll_net > $n.unf.out 2> $n.unf.err &
##	punf -n=200000 -N=1 -s -t -@4 -# -M $n.ll_net > $n.mp.out 2> $n.mp.err &
##
##	echo $n >> out
##	grep 'Net statistics' < $n.unf.err | sed 's/Net statistics: /net /' >> out
##	grep 'Unfolding statistics' < $n.unf.err | sed 's/Unfolding statistics: /unf /' >> out
##	grep 'Unfolding statistics' < $n.mp.err | sed 's/Unfolding statistics: /mp  /' >> out
##done
#
#echo >> out
#echo "PR unfolding and MP" >> out
#for n in $PLAIN; do
#
#	echo "PR $n"
#	punf -n=200000 -r -N=1 -s -t -@4 -# $n.ll_net > $n.prunf.out 2> $n.prunf.err
#	punf -n=200000 -r -N=1 -s -t -@4 -# -M $n.ll_net > $n.prmp.out 2> $n.prmp.err 
#
#	echo "PR $n" >> out
#	grep 'Net statistics' < $n.prunf.err | sed 's/Net statistics: /net /' >> out
#	grep 'Unfolding statistics' < $n.prunf.err | sed 's/Unfolding statistics: /unf /' >> out
#	grep 'Unfolding statistics' < $n.prmp.err | sed 's/Unfolding statistics: /mp  /' >> out
#done
#
#echo >> out
#echo "Contextual unfolding and CMPs (E, B, EH, BH)" >> out
#for n in $PLAIN; do
#	./prcompress $n.mci > $n.prcompress.out 2> $n.prcompress.err
#	tools/merger.py < $n.pr.cuf > $n.cont.mp 2> $n.merger.err
#	E=`grep ^events < $n.merger.err | sed 's/events.//'`
#	B=`grep conds < $n.merger.err | sed 's/conds.//'`
#
#	EH=`grep mp-evs < $n.merger.err | sed 's/mp-evs.//'`
#	BH=`grep mp-cond < $n.merger.err | sed 's/mp-cond.//'`
#	echo "$E\t$B\t$EH\t$BH\t$n" >> out
#done

GOALS="
test/nets/plain/small/do_od.punf.c.txt
test/nets/plain/small/dme4.punf.c.txt
test/nets/plain/small/cottbus_plate_5.punf.c.txt
test/nets/plain/small/stack_full.punf.c.txt
test/nets/plain/small/elevator_2.punf.c.txt
test/nets/plain/small/sentest_75.fsa.punf.c.txt
test/nets/plain/small/mutual.punf.c.txt
test/nets/plain/small/sdl_example.punf.c.txt
test/nets/plain/small/rrr10-1.sync.punf.c.txt
test/nets/plain/small/ab_gesc.punf.c.txt
test/nets/plain/small/sentest_25.fsa.punf.c.txt
test/nets/plain/small/gas_station.punf.c.txt
test/nets/plain/small/elevator_1.fsa.punf.c.txt
test/nets/plain/small/sentest_50.fsa.punf.c.txt
test/nets/plain/small/rw_1w1r.punf.c.txt
test/nets/plain/small/eisenbahn.sync.punf.c.txt
test/nets/plain/small/mmgt_1.fsa.punf.c.txt
test/nets/plain/small/recursion.punf.c.txt
test/nets/plain/small/peterson.punf.c.txt
test/nets/plain/small/sdl_arq.punf.c.txt
test/nets/plain/small/key_2.fsa.punf.c.txt
test/nets/plain/small/byzagr4_0b.punf.c.txt
test/nets/plain/small/dijkstra_2.sync.punf.c.txt
test/nets/plain/small/mmgt_2.fsa.punf.c.txt
test/nets/plain/small/byzagr4_0b.sync.punf.c.txt
test/nets/plain/small/elevator.punf.c.txt
test/nets/plain/small/rrr20-1.sync.punf.c.txt
test/nets/plain/small/rw_1w1r.sync.punf.c.txt
test/nets/plain/small/elevator_1.punf.c.txt
test/nets/plain/small/reader_writer_2.punf.c.txt
test/nets/plain/small/parrow.punf.c.txt
test/nets/plain/small/sentest_100.fsa.punf.c.txt
test/nets/plain/small/dme2.punf.c.txt
test/nets/plain/small/furnace_1.fsa.punf.c.txt
test/nets/plain/small/only_hl.punf.c.txt
test/nets/plain/small/byzagr4_2a.punf.c.txt
test/nets/plain/small/sdl_arq_deadlock.punf.c.txt
test/nets/plain/small/rrr50-1.sync.punf.c.txt
test/nets/plain/small/byzagr4_2a.sync.punf.c.txt
test/nets/plain/small/rrr30-1.sync.punf.c.txt
test/nets/plain/small/elevator_2.fsa.punf.c.txt
test/nets/plain/small/dijkstra_2.punf.c.txt
test/nets/plain/small/eisenbahn.punf.c.txt
test/nets/plain/small/abp_1.fsa.punf.c.txt
test/nets/plain/small/peterson_pfa.punf.c.txt
test/nets/plain/small/cottbus_plate_5.sync.punf.c.txt
test/nets/plain/small/key_2.punf.c.txt
test/nets/plain/small/dme3.punf.c.txt
test/nets/plain/med/q_1.fsa.punf.c.txt
test/nets/plain/med/dme7.punf.c.txt
test/nets/plain/med/elevator_3.punf.c.txt
test/nets/plain/med/dme9.punf.c.txt
test/nets/plain/med/dme10.punf.c.txt
test/nets/plain/med/knuth_2.punf.c.txt
test/nets/plain/med/elevator_3.dlmcs.sync.punf.c.txt
test/nets/plain/med/dme6.punf.c.txt
test/nets/plain/med/rw_1w2r.punf.c.txt
test/nets/plain/med/rw_2w1r.sync.punf.c.txt
test/nets/plain/med/dme8.punf.c.txt
test/nets/plain/med/mmgt_3.fsa.punf.c.txt
test/nets/plain/med/bruijn_2.sync.punf.c.txt
test/nets/plain/med/rw_2w1r.punf.c.txt
test/nets/plain/med/speed_1.fsa.punf.c.txt
test/nets/plain/med/elevator_3.fsa.punf.c.txt
test/nets/plain/med/key_3.fsa.punf.c.txt
test/nets/plain/med/furnace_2.fsa.punf.c.txt
test/nets/plain/med/key_3.punf.c.txt
test/nets/plain/med/dme11.punf.c.txt
test/nets/plain/med/q_1.punf.c.txt
test/nets/plain/med/knuth_2.sync.punf.c.txt
test/nets/plain/med/dme5.punf.c.txt
test/nets/plain/med/bds_1.fsa.punf.c.txt
test/nets/plain/med/bruijn_2.punf.c.txt
test/nets/plain/large/elevator_4.old.punf.c.txt
test/nets/plain/large/rw_1w3r.punf.c.txt
test/nets/plain/large/elevator_4.dlmcs.sync.punf.c.txt
test/nets/plain/large/furnace_3.fsa.punf.c.txt
test/nets/plain/large/ftp_1.sync.punf.c.txt
test/nets/plain/large/key_3.sync.punf.c.txt
test/nets/plain/large/rw_12.sync.punf.c.txt
test/nets/plain/large/rw_12.punf.c.txt
test/nets/plain/large/elevator_4.punf.c.txt
test/nets/plain/large/q_1.sync.punf.c.txt
test/nets/plain/large/furnace_4.punf.c.txt
test/nets/plain/large/bds_1.sync.punf.c.txt
test/nets/plain/large/dpd_7.sync.punf.c.txt
test/nets/plain/large/rw_1w3r.sync.punf.c.txt
test/nets/plain/large/key_4.punf.c.txt
test/nets/plain/large/furnace_3.punf.c.txt
test/nets/plain/large/mmgt_4.fsa.punf.c.txt
test/nets/plain/large/elevator_4.fsa.punf.c.txt
test/nets/plain/large/ftp_1.fsa.punf.c.txt
test/nets/plain/large/byzagr4_1b.punf.c.txt
test/nets/plain/large/key_4.fsa.punf.c.txt
"

#make -n -j 20 $GOALS

echo
date
echo




FALTAN="
test/nets/plain/large/ftp_1.sync.punf.r.c.txt
test/nets/plain/large/furnace_3.punf.r.c.txt
test/nets/plain/large/mmgt_4.fsa.punf.r.c.txt
test/nets/plain/large/elevator_4.fsa.punf.r.c.txt
test/nets/plain/large/ftp_1.fsa.punf.r.c.txt
test/nets/plain/large/byzagr4_1b.punf.r.c.txt
test/nets/plain/large/key_4.fsa.punf.r.c.txt
test/nets/plain/large/key_4.punf.r.c.txt
test/nets/plain/large/furnace_3.punf.c.txt
test/nets/plain/large/mmgt_4.fsa.punf.c.txt
test/nets/plain/large/elevator_4.fsa.punf.c.txt
test/nets/plain/large/ftp_1.fsa.punf.c.txt
test/nets/plain/large/byzagr4_1b.punf.c.txt
test/nets/plain/large/key_4.punf.c.txt
test/nets/plain/large/key_4.fsa.punf.c.txt
"

for f in $FALTAN; do
	grep '^ Net file: ' < $f
	grep '^ Net statistics: ' < $f
	grep '^ Unfolding statistics: ' < $f
	grep '^events	' < $f
	grep '^conds	' < $f
	grep '^mp-evs	' < $f
	grep '^mp-cond	' < $f
done

