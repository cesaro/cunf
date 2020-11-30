#!/usr/bin/env python3

'''
N places p_i, initially marked, are read by N transitions t_i which consume from
q_i.  A single transition t consumes all p_i.  Contextual output.
'''

import sys
import ptnet

def mkcmpread (n) :
    assert (n >= 1)
    net = ptnet.net.Net ()
    t = ptnet.net.Transition ('t')
    net.trans.append (t)

    for i in range (n) :
        q = ptnet.net.Place ('q/%d' % i)
        p = ptnet.net.Place ('p/%d' % i)
        net.m0[q] = 1
        net.m0[p] = 1

        u = ptnet.net.Transition ('t/%d' % i)
        u.pre_add (q)
        u.cont_add (p)

        t.pre_add (p)

        net.places.append (q)
        net.places.append (p)
        net.trans.append (u)

    net.write (sys.stdout, 'pep')

if __name__ == '__main__' :
    assert (len (sys.argv) == 2)
    n = int (sys.argv[1])
    mkcmpread (n)

# vi:ts=4:sw=4:et:
