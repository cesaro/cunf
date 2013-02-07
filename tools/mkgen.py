#!/usr/bin/env python

'''
Produces n-Gen c-nets, consisting in n processes that nondeterministically
execute t_i or t'_i to produce a resource r_i.  All transitions t_i have p in
their context, and all t'_i have p' in their context.
All places r_i are consumed by a transition t
'''

import sys
import ptnet

def mkcmpread (n) :
    assert (n >= 1)
    net = ptnet.net.Net ()
    net.author = 'Stefan Schwoon'
    net.title = 'A good example for CMPs :)'
    net.note = 'Generated with mkgen.py'

    s = []
    v = []
    for i in range (4) :
        s.append (net.place_add ('s%d' % i))
        v.append (net.trans_add ('v%d' % i))

    p1 = net.place_add ('p1')
    p2 = net.place_add ('p2')

    net.m0_add (s[0])
    net.m0_add (s[1])

    s[0].post_add (v[0])
    s[1].post_add (v[1])
    v[0].post_add (p1)
    v[1].post_add (p2)
    p1.post_add (v[2])
    p2.post_add (v[3])
    v[2].post_add (s[2])
    v[3].post_add (s[3])

    r = []
    for i in xrange (n) :
        q = net.place_add ('q/%d' % i, 1)
        r.append (net.place_add ('r/%d' % i))
        t = net.trans_add ('t/%d' % i)
        u = net.trans_add ('u/%d' % i)
        net.m0_add (q)

        t.pre_add (q)
        t.post_add (r[i])
        t.cont_add (p1)

        u.pre_add (q)
        u.post_add (r[i])
        u.cont_add (p2)

    t = net.trans_add ('t')
    p = net.place_add ('p')
    p.pre_add (t)
    for i in xrange (n) :
        t.pre_add (r[i])
    
    net.cont2plain ()
    net.write (sys.stdout, 'pep')

if __name__ == '__main__' :
    assert (len (sys.argv) == 2)
    n = int (sys.argv[1])
    mkcmpread (n)

# vi:ts=4:sw=4:et:
