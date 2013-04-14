#!/usr/bin/env python

'''
Produces basic flat arbiters like those in Fig. 5 in:
  Mokhov, Khomenko, Yakovlev. Flat Arbiters. Fundamenta Informaticae 108 (2011)
'''

import sys
import ptnet

def mk_me_impl (net, tr, pl, r1, r2, g1, g2) :
    #tr = {}
    #pl = {}
    #for s in [r1, r2, g1, g2] :
    #    for e in ['+', '-'] :
    #        tr[s, e] = net.trans_add (s + e)
    #    for e in ['0', '1'] :
    #        pl[s, e] = net.place_add (s + e)
    #for s in [r1, r2, g1, g2] :

    #print 'ME', r1, r2, g1, g2
    for s in [g1, g2] :
        tr[s, '+'].post_add (pl[s, '1'])
        tr[s, '+'].pre_add (pl[s, '0'])
        tr[s, '-'].pre_add (pl[s, '1'])
        tr[s, '-'].post_add (pl[s, '0'])

    tr[g1, '+'].cont_add (pl[r1, '1'])
    tr[g1, '+'].cont_add (pl[g2, '0'])

    tr[g2, '+'].cont_add (pl[r2, '1'])
    tr[g2, '+'].cont_add (pl[g1, '0'])

    tr[g1, '-'].cont_add (pl[r1, '0'])
    tr[g2, '-'].cont_add (pl[r2, '0'])
    
def mk_ce_impl (net, tr, pl, inputs, output) :
    #print 'C-elem', inputs, output
    tr[output, '+'].pre_add (pl[output, '0'])
    tr[output, '+'].post_add (pl[output, '1'])
    tr[output, '-'].pre_add (pl[output, '1'])
    tr[output, '-'].post_add (pl[output, '0'])
    for s in inputs :
        tr[output, '+'].cont_add (pl[s, '1'])
        tr[output, '-'].cont_add (pl[s, '0'])

def mk_cli_env (net, tr, pl, i, r, g) :
    #print 'cli', r, g
    p0 = net.place_add ('p0/%d' % i, 1)
    p1 = net.place_add ('p1/%d' % i, 0)
    p2 = net.place_add ('p2/%d' % i, 0)
    p3 = net.place_add ('p3/%d' % i, 0)

    tr[r,'+'].pre_add (p0)
    tr[r,'+'].post_add (p1)
    tr[g,'+'].pre_add (p1)
    tr[g,'+'].post_add (p2)
    tr[r,'-'].pre_add (p2)
    tr[r,'-'].post_add (p3)
    tr[g,'-'].pre_add (p3)
    tr[g,'-'].post_add (p0)

def sig (n, i) :
    return n + str (i)

def mk_arbiter_basic (n) :
    assert (n >= 3)
    net = ptnet.net.Net ()

    tr = {}
    pl = {}

    # make request and grant signals, and signal 'edges'
    for i in range (n) :
        for rg in ['r', 'g'] :
            s = sig (rg, i)
            for v in ['0', '1'] :
                pl[s, v] = net.place_add ('%s=%s' % (s, v))
            for v in ['+', '-'] :
                tr[s, v] = net.trans_add (s + v)

    # make signals and signal edges for the outpus of the MEs
    for i in range (n) :
        for j in range (n) :
            if j == i : continue
            s = 'me%d,%d' % (i,j)
            for v in ['0', '1'] :
                pl[s, v] = net.place_add ('%s=%s' % (s, v))
            for v in ['+', '-'] :
                tr[s, v] = net.trans_add (s + v)

    # all signals are initially low
    for s, v in pl :
        if v == '0' : net.m0_add (pl[s, v])

    # for each i != j, make the corresponding ME
    for i in range (n) :
        for j in range (i + 1, n) :
            mk_me_impl (net, tr, pl, sig('r',i), sig('r',j),
                    'me%d,%d' % (i,j), 'me%d,%d' % (j,i))

    # make the c-elements
    for i in range (n) :
        l = ['me%d,%d' % (i, j) for j in range (n) if j != i]
        mk_ce_impl (net, tr, pl, l, sig ('g', i))

    # for request (input) signals, make the most general environment
    for i in range (n) :
        tr[sig ('r', i), '+'].post_add (pl[sig ('r', i), '1'])
        tr[sig ('r', i), '+'].pre_add (pl[sig ('r', i), '0'])
        tr[sig ('r', i), '-'].pre_add (pl[sig ('r', i), '1'])
        tr[sig ('r', i), '-'].post_add (pl[sig ('r', i), '0'])

    # make the environment for each client
    for i in range (n) :
        mk_cli_env (net, tr, pl, i, sig('r',i), sig('g',i))

    #net.cont2plain ()
    net.write (sys.stdout, 'pep')

if __name__ == '__main__' :
    assert (len (sys.argv) == 2)
    n = int (sys.argv[1])
    mk_arbiter_basic (n)

# vi:ts=4:sw=4:et:
