#!/usr/bin/env python

'''
A resource p is read by N concurrent transitions t_i which consume from private
resources r_i and produce private resources s_i.

Resource p, and private resources r_i are initially marked, and p is consumed
by t.
'''

import sys
import ptnet

def mkread (n) :
    assert (n >= 1)
    net = ptnet.net.Net ()

    p = ptnet.net.Place ('p')
    q = ptnet.net.Place ('q')
    t = ptnet.net.Transition ('t')
    t.pre_add (p)
    t.post_add (q)
    p.m0 = True
    net.places.add (p)
    net.places.add (q)
    net.trans.add (t)

    for i in xrange (n) :
        r = ptnet.net.Place ('r/%d' % i)
        s = ptnet.net.Place ('s/%d' % i)
        r.m0 = True

        t = ptnet.net.Transition ('t/%d' % i)
        t.pre_add (r)
        t.post_add (s)
        t.cont_add (p)

        net.places.add (r)
        net.places.add (s)
        net.trans.add (t)

    net.write (sys.stdout, 'pep')

if __name__ == '__main__' :
    assert (len (sys.argv) == 2)
    n = int (sys.argv[1])
    mkread (n)

# vi:ts=4:sw=4:et:
