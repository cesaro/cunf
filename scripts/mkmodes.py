#!/usr/bin/env python

'''
Model of a system with N modes, each of one enables the execution of M distinct
concurrent processes.  The system can only be in one mode at a time.
Contextual output.
'''

import sys
import ptnet

def mkstage (net, p, i, m) :
    for j in range (m) :
        pi = ptnet.net.Place ('p/%d/%d' % (i, j))
        qi = ptnet.net.Place ('q/%d/%d' % (i, j))
        ti = ptnet.net.Transition ('t/%d/%d' % (i, j))
        ui = ptnet.net.Transition ('u/%d/%d' % (i, j))
        pi.m0 = True

        net.places.append (pi)
        net.places.append (qi)
        net.trans.append (ti)
        net.trans.append (ui)

        ti.pre_add (pi)
        ti.cont_add (p)
        ti.post_add (qi)

        ui.pre_add (qi)
        ui.cont_add (p)
        ui.post_add (pi)

def mkmode (n, m) :
    assert (n >= 1)
    assert (m >= 1)

    net = ptnet.net.Net ()
    modes = []
    for i in range (n) :
        p = ptnet.net.Place ('mode/%d' % i)
        modes.append (p)
        net.places.append (p)
        mkstage (net, p, i, m)
        if i >= 1 :
            ti = ptnet.net.Transition ('mode/up/%d' % i)
            ui = ptnet.net.Transition ('mode/down/%d' % i)
            net.trans.append (ti)
            net.trans.append (ui)

            ti.pre_add (modes[i - 1])
            ti.post_add (p)
            ui.pre_add (p)
            ui.post_add (modes[i - 1])
    
    net.m0[modes[0]] = 1
    net.write (sys.stdout, 'pep')

if __name__ == '__main__' :
    assert (len (sys.argv) == 3)
    n = int (sys.argv[1])
    m = int (sys.argv[2])
    mkmode (n, m)

# vi:ts=4:sw=4:et:
