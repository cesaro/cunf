#!/usr/bin/env python3

'''
Model of a transmission line of binary values with N nodes.  The (i+1)-th node
repeats what the i-th node says.  All nodes are initially low (!).  The output
is a contextual net.
'''

import sys

def mksignal (n) :

    assert (n >= 1)

    nrp = 1
    nrt = 1
    low = {}
    high = {}
    fall = {}
    rise = {}

    for i in range (n + 1) :
        low[i] = nrp
        high[i] = nrp + 1
        fall[i] = nrt
        rise[i] = nrt + 1
        nrp += 2
        nrt += 2

    print 'PEP\nPetriBox\nFORMAT_N2\nPL'
    for i in range (n + 1) :
        print '%d"low%d"M1m1' % (low[i], i)
        print '%d"high%d"' % (high[i], i)

    print 'TR'
    for i in range (n + 1) :
        print '%d"fall%d"' % (fall[i], i)
        print '%d"rise%d"' % (rise[i], i)

    print 'TP'
    for i in range (n + 1) :
        print '%d<%d' % (fall[i], low[i])
        print '%d<%d' % (rise[i], high[i])
        
    print 'PT'
    for i in range (n + 1) :
        print '%d>%d' % (low[i], rise[i])
        print '%d>%d' % (high[i], fall[i])
        
    print 'RA'
    for i in range (1, n + 1) :
        print '%d<%d' % (fall[i], low[i - 1])
        print '%d<%d' % (rise[i], high[i - 1])


if __name__ == '__main__' :
    assert (len (sys.argv) == 2)
    n = int (sys.argv[1])
    mksignal (n)

# vi:ts=4:sw=4:et:
