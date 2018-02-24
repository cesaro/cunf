#!/usr/bin/env python

'''
Conway's Game of Life on a NxN square grid with the Von Neumann neighborhood
(north, south, east, west).

The grid is actually at the center of a (N+2)x(N+2) grid, where edge cells are
initially fixed and cannot die or be born.  Every inner cell has therefore 4
neighbors.  Two rules are available.  The initial state is randomly chosen, and
is the only randomized feature of the net.  The output is a contextual net.
'''

import sys
import random
import ptnet

def b3s23 () :
    l = []

    # B3 / S23
    # name, pre, post, north, east, south, west
    # die if 0 or 4
    l.append (('death0', 1, 0, 0, 0, 0, 0))
    l.append (('death4', 1, 0, 1, 1, 1, 1))

    # die if 1
    l.append (('death1no', 1, 0, 1, 0, 0, 0))
    l.append (('death1ea', 1, 0, 0, 1, 0, 0))
    l.append (('death1so', 1, 0, 0, 0, 1, 0))
    l.append (('death1we', 1, 0, 0, 0, 0, 1))

    # birth if 3
    l.append (('birth3no', 0, 1, 0, 1, 1, 1))
    l.append (('birth3ea', 0, 1, 1, 0, 1, 1))
    l.append (('birth3so', 0, 1, 1, 1, 0, 1))
    l.append (('birth3we', 0, 1, 1, 1, 1, 0))
    return l

def b24s13 () :
    l = []

    # B24 / S13
    # name, pre, post, north, east, south, west
    # birth if 4
    l.append (('birth4', 0, 1, 1, 1, 1, 1))

    # birth if 2
    l.append (('birth2/1', 0, 1, 1, 1, 0, 0))
    l.append (('birth2/2', 0, 1, 1, 0, 1, 0))
    l.append (('birth2/3', 0, 1, 1, 0, 0, 1))

    l.append (('birth2/4', 0, 1, 0, 1, 1, 0))
    l.append (('birth2/5', 0, 1, 0, 1, 0, 1))

    l.append (('birth2/6', 0, 1, 0, 0, 1, 1))

    # die if 0 or 4
    l.append (('death0', 1, 0, 0, 0, 0, 0))
    l.append (('death4', 1, 0, 1, 1, 1, 1))

    # die if 2
    l.append (('death2/1', 1, 0, 1, 1, 0, 0))
    l.append (('death2/2', 1, 0, 1, 0, 1, 0))
    l.append (('death2/3', 1, 0, 1, 0, 0, 1))

    l.append (('death2/4', 1, 0, 0, 1, 1, 0))
    l.append (('death2/5', 1, 0, 0, 1, 0, 1))

    l.append (('death2/6', 1, 0, 0, 0, 1, 1))
    return l

def no (x, y) :
    return (x, y - 1)

def we (x, y) :
    return (x - 1, y)

def so (x, y) :
    return (x, y + 1)

def ea (x, y) :
    return (x + 1, y)

def mklife (n) :
    assert (n >= 1)
    net = ptnet.net.Net ()
    tab = {}

    for x in xrange (n + 2) :
        for y in xrange (n + 2) :
            pon = ptnet.net.Place ('on/%dx%d' % (x, y))
            poff = ptnet.net.Place ('off/%dx%d' % (x, y))
            net.places.append (pon)
            net.places.append (poff)
            tab[x,y] = (poff, pon)

#    rules = b3s23 ()
    rules = b24s13 ()
    for x in xrange (1, n + 1) :
        for y in xrange (1, n + 1) :
            for r in rules :
                t = ptnet.net.Transition ('%s/%dx%d' % (r[0], x, y))
                net.trans.append (t)
                t.pre_add (tab[x,y][r[1]])
                t.post_add (tab[x,y][r[2]])
                t.cont_add (tab[no (x, y)][r[3]])
                t.cont_add (tab[ea (x, y)][r[4]])
                t.cont_add (tab[so (x, y)][r[5]])
                t.cont_add (tab[we (x, y)][r[6]])

    seed = random.randint (1, 999)
    print 'seed', seed
    random.seed (seed)
    for x in xrange (n + 2) :
        for y in xrange (n + 2) :
            b = random.randint (0, 3)
#            b = 1
            tab[x,y][0 if b else 1].m0 = True

#    for z in xrange (1, n + 1) :
#        tab[0,z][1].m0 = True
#        tab[0,z][0].m0 = False
#        tab[z,0][1].m0 = True
#        tab[z,0][0].m0 = False

    for x in xrange (n + 2) :
        for y in xrange (n + 2) :
            assert (tab[x,y][0].m0 != tab[x,y][1].m0)
            sys.stdout.write ('x ' if tab[x,y][1].m0 else '- ')
        sys.stdout.write ('\n')
    net.write (sys.stderr, 'pep')

if __name__ == '__main__' :
    assert (len (sys.argv) == 2)
    n = int (sys.argv[1])
    mklife (n)

# vi:ts=4:sw=4:et:
