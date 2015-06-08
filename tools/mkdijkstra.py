#!/usr/bin/env python

'''
Produces c-nets encoding Dijkstra's mutual exclusion algorithm, following his
paper from 1965.
'''

import sys
import ptnet

def mkdijkstra (n) :
    assert (n >= 1)
    net = ptnet.net.Net ()
    net.author = 'E. W. Dijkstra'
    net.title = 'Djikstra\'s mutual exclusion protocol (1965)'
    net.note = ''

    # make global variables: b, c, k; b[i], c[i] start at T, k=0
    b = {}
    c = {}
    k = {}
    for i in range (n) :
        b[i, 'F'] = net.place_add ('b[%d]=F' % i)
        b[i, 'T'] = net.place_add ('b[%d]=T' % i, 1)
        c[i, 'F'] = net.place_add ('c[%d]=F' % i)
        c[i, 'T'] = net.place_add ('c[%d]=T' % i, 1)
        k[i] = net.place_add ('k=%d' % i)
    net.m0[k[0]] = 1

    # make all processes
    for i in range (n) :
        mkdijproc (net, i, n, b, c, k)

    #net.cont2plain ()
    net.write (sys.stdout, 'pep')

def mkdijproc (net, i, n, b, c, k) :

    # instruction pointers
    l = {}
    for j in range (7) :
        l[j] = net.place_add ('l%d/%i' % (j, i))
    net.m0[l[0]] = 1

    t = net.trans_add ('b[%d]:=F' % i)
    t.pre_add (l[0])
    t.pre_add (b[i, 'T'])
    t.post_add (b[i, 'F'])
    t.post_add (l[1])

    t = net.trans_add ('k=%d? goto l4' % i)
    t.pre_add (l[1])
    t.cont_add (k[i])
    t.post_add (l[4])

    for j in range (n) :
        if j == i : continue
        t = net.trans_add ('k=%d && b[k]=T? goto l3/%d' % (j, i))
        t.pre_add (l[1])
        t.cont_add (k[j])
        t.cont_add (b[j, 'T'])
        t.post_add (l[3])

        t = net.trans_add ('k=%d? k:=%d' % (j, i))
        t.pre_add (l[3])
        t.pre_add (k[j])
        t.post_add (k[i])
        t.post_add (l[1])

    t = net.trans_add ('c[%d]:=F' % i)
    t.pre_add (l[4])
    t.pre_add (c[i, 'T'])
    t.post_add (c[i, 'F'])
    t.post_add (l[5])

    t = net.trans_add ('forall j!=%d, c[j]=T? goto l6' % i)
    t.pre_add (l[5])
    for j in range (n) :
        if j == i : continue
        t.cont_add (c[j, 'T'])
    t.post_add (l[6])

    for j in range (n) :
        if j == i : continue
        t = net.trans_add ('c[%d]=F? goto l2/%d' % (j, i))
        t.pre_add (l[5])
        t.cont_add (c[j, 'F'])
        t.post_add (l[2])

    t = net.trans_add ('c[%d]:=T' % i)
    t.pre_add (l[2])
    t.pre_add (c[i, 'F'])
    t.post_add (c[i, 'T'])
    t.post_add (l[1])

    t = net.trans_add ('c[%d]:=T; b[%d]:=T' % (i, i))
    t.pre_add (l[6])
    t.pre_add (c[i, 'F'])
    t.post_add (c[i, 'T'])
    t.pre_add (b[i, 'F'])
    t.post_add (b[i, 'T'])
    t.post_add (l[0])

if __name__ == '__main__' :
    assert (len (sys.argv) == 2)
    n = int (sys.argv[1])
    mkdijkstra (n)

# vi:ts=4:sw=4:et:
