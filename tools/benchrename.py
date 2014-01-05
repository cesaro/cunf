#!/usr/bin/env python

import sys

m = {
'large/bds_1.fsa'           : '\\textsc{Bds(1)}',
'large/byzagr4_1b'          : '\\textsc{Byz}',
'large/ftp_1.sync'          : '\\textsc{Ftp}',
'med/bruijn_2.fsa'          : '\\textsc{Bruj(2)}',

'small/key_2'               : '\\textsc{Key(2)}',
'med/key_3'                 : '\\textsc{Key(3)}',
'large/key_4'               : '\\textsc{Key(4)}',

'dij02'                     : '\\textsc{Dij(02)}',
'dij03'                     : '\\textsc{Dij(03)}',
'dij04'                     : '\\textsc{Dij(04)}',
'dij05'                     : '\\textsc{Dij(05)}',
'dij05'                     : '\\textsc{Dij(05)}',
'dij07'                     : '\\textsc{Dij(07)}',
'dij08'                     : '\\textsc{Dij(08)}',

'dij10'                     : '\\textsc{Dek(10)}',
'dij20'                     : '\\textsc{Dek(20)}',
'dij30'                     : '\\textsc{Dek(30)}',
'dij40'                     : '\\textsc{Dek(40)}',
'dij50'                     : '\\textsc{Dek(50)}',
'dij60'                     : '\\textsc{Dek(60)}',
'dij80'                     : '\\textsc{Dek(80)}',

'dme2'                      : '\\textsc{Dme(02)}',
'dme3'                      : '\\textsc{Dme(03)}',
'dme4'                      : '\\textsc{Dme(04)}',
'dme5'                      : '\\textsc{Dme(05)}',
'dme6'                      : '\\textsc{Dme(06)}',
'dme7'                      : '\\textsc{Dme(07)}',
'dme8'                      : '\\textsc{Dme(08)}',
'dme9'                      : '\\textsc{Dme(09)}',
'dme10'                     : '\\textsc{Dme(10)}',
'dme11'                     : '\\textsc{Dme(11)}',
'dme12'                     : '\\textsc{Dme(12)}',

'elevator_1'                     : '\\textsc{Elev(1)}',
'elevator_2'                     : '\\textsc{Elev(2)}',
'elevator_3'                     : '\\textsc{Elev(3)}',
'elevator_4'                     : '\\textsc{Elev(4)}',

'rw_1w3r.ll_net'                : '\\textsc{RW(1,3)}',
'rw_1w2r.ll_net'                : '\\textsc{RW(1,2)}',
'rw_2w1r.ll_net'                : '\\textsc{RW(2,1)}',
'rw_1w1r.ll_net'                : '\\textsc{RW(1,1)}',

'sentest_25'                     : '\\textsc{Sent(25)}',
'sentest_50'                     : '\\textsc{Sent(50)}',
'sentest_75'                     : '\\textsc{Sent(75)}',
'sentest_100'                    : '\\textsc{Sent(100)}',

'mmgt_1'                     : '\\textsc{Mmgt(1)}',
'mmgt_2'                     : '\\textsc{Mmgt(2)}',
'mmgt_3'                     : '\\textsc{Mmgt(3)}',
'mmgt_4'                     : '\\textsc{Mmgt(4)}',

'byzagr4_1b.ll_net'          : '\\textsc{Byz}',
'ftp_1.fsa.ll_net'           : '\\textsc{Ftp}',

'byzagr4_1b.ll_net'          : '\\textsc{Byz}',

}

# sync examples:
# cont/large/bds_1.sync.ll_net
# cont/large/dpd_7.sync.ll_net
# cont/large/ftp_1.sync.ll_net
# cont/large/q_1.sync.ll_net
# cont/large/rw_12.sync.ll_net
# cont/large/rw_1w3r.sync.ll_net
# cont/med/bruijn_2.sync.ll_net
# 
# cont/med/knuth_2.sync.ll_net
# cont/med/rw_2w1r.sync.ll_net
# 
# cont/small/byzagr4_0b.sync.ll_net
# cont/small/byzagr4_2a.sync.ll_net
# cont/small/cottbus_plate_5.sync.ll_net
# cont/small/eisenbahn.sync.ll_net
# cont/small/rrr10-1.sync.ll_net
# cont/small/rrr20-1.sync.ll_net
# cont/small/rrr30-1.sync.ll_net
# cont/small/rrr50-1.sync.ll_net
# cont/small/rw_1w1r.sync.ll_net
# 
# 
# do not follow the series:
# cont/med/q_1.fsa.ll_net
# cont/med/q_1.ll_net
# 
# cont/small/furnace_1.fsa.ll_net
# cont/med  /furnace_2.fsa.ll_net
# cont/large/furnace_3.fsa.ll_net
# cont/large/furnace_3.ll_net
# cont/large/furnace_4.ll_net
# 
# 
# # they are too small
# cont/med/knuth_2.ll_net
# cont/med/speed_1.fsa.ll_net
# cont/small/ab_gesc.ll_net
# cont/small/abp_1.fsa.ll_net
# cont/small/byzagr4_0b.ll_net
# cont/small/byzagr4_2a.ll_net
# cont/small/cottbus_plate_5.ll_net
# cont/small/dijkstra_2.ll_net
# cont/small/do_od.ll_net
# cont/small/eisenbahn.ll_net
# cont/small/gas_station.ll_net
# cont/small/mutual.ll_net
# cont/small/only_hl.ll_net
# cont/small/parrow.ll_net
# cont/small/peterson.ll_net
# cont/small/peterson_pfa.ll_net
# cont/small/reader_writer_2.ll_net
# cont/small/recursion.ll_net
# cont/small/sdl_arq.ll_net
# cont/small/sdl_arq_deadlock.ll_net
# cont/small/sdl_example.ll_net
# cont/small/stack_full.ll_net

def main () :
    for line in sys.stdin :
        found = 0
        line = line.strip ()
        for k in m :
            if k in line :
                found = 1
                break
        if found :
            print '%s\t\t\t%s' % (m[k], line)
        else :
            print '%s\t\t\t%s' % (line, line)

if __name__ == '__main__' :
    main ()


# vi:ts=4:sw=4:et:
