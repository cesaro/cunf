#!/usr/bin/env python

'''
A resource p is read by N concurrent transitions t_i which consume from private
resources r_i and produce private resources s_i.

Private resources r_i are initially marked, p is generated by transition u and
consumed by t.
'''

import sys
import ptnet

def mkread (n) :
    assert (n >= 1)
    net = ptnet.net.Net ()

    r = ptnet.net.Place ('r')
    p = ptnet.net.Place ('p')
    s = ptnet.net.Place ('s')

    u = ptnet.net.Transition ('u')
    t = ptnet.net.Transition ('t')
    u.pre_add (r)
    u.post_add (p)
    t.pre_add (p)
    t.post_add (s)

    r.m0 = True
    net.places.append (r)
    net.places.append (p)
    net.places.append (s)
    net.trans.append (t)
    net.trans.append (u)

    for i in xrange (n) :
        r = ptnet.net.Place ('r/%d' % i)
        s = ptnet.net.Place ('s/%d' % i)
        r.m0 = True

        t = ptnet.net.Transition ('t/%d' % i)
        t.pre_add (r)
        t.post_add (s)
        t.cont_add (p)

        net.places.append (r)
        net.places.append (s)
        net.trans.append (t)

    net.write (sys.stdout, 'pep')

if __name__ == '__main__' :
    assert (len (sys.argv) == 2)
    n = int (sys.argv[1])
    mkread (n)

# vi:ts=4:sw=4:et:
