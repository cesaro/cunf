#!/usr/bin/env python

'''
Builds a c-net representing a variant of the Dekker's agorithm on n processses,
with n >= 2.  Each process has three states, p0, p1, and p3. p0 is initial.
From there, the process executes 'try' and raises its flag, reaching p1.
In p1, if at least one of the other process has a high flag, it 'withdraw's its
intent and goes back to p0.  In p1, it 'enter's the critical section if all
other process' flag is zero.  From p3, the process can only 'exit' the critical
section, reseting its flag.

Mutual exclusion and deadlock-freedom is guaranted.  Unfair runs are possible.
'''

import sys
import ptnet

def mkproc (i, net, n, flag) :
    intent = ptnet.net.Transition ('try/%d' % i)
    withdraw = {}
    for j in range (n) :
        if j == i : continue
        withdraw[j] = ptnet.net.Transition ('withdraw/%d/%d' % (i, j))
        net.trans.append (withdraw[j])
    enter = ptnet.net.Transition ('enter/%d' % i)
    exit = ptnet.net.Transition ('exit/%d' % i)
    for t in [intent, enter, exit] :
        net.trans.append (t)

    p0 = ptnet.net.Place ('p0/%d' % i)
    p1 = ptnet.net.Place ('p1/%d' % i)
    p3 = ptnet.net.Place ('p3/%d' % i)
    for p in [p0, p1, p3] :
        net.places.append (p)

    # p0 is my starting state
    p0.m0 = 1
    net.m0.add (p0)

    # set my flag to 1
    intent.pre_add (p0)
    intent.pre_add (flag[i, 0])
    intent.post_add (flag[i, 1])
    intent.post_add (p1)

    # if the flag of any other process is 1, set my flag to 0
    for j in range (n) :
        if j == i : continue
        withdraw[j].pre_add (p1)
        withdraw[j].pre_add (flag[i, 1])
        withdraw[j].post_add (flag[i, 0])
        withdraw[j].post_add (p0)
        withdraw[j].cont_add (flag[j, 1])

    # i can enter the critical section (p3) if all flags are 0 except mine
    enter.pre_add (p1)
    enter.post_add (p3)
    for j in range (n) :
        if i != j : enter.cont_add (flag[j, 0])

    # i exit the critical section seting my flag to 0
    exit.pre_add (p3)
    exit.pre_add (flag[i, 1])
    exit.post_add (flag[i, 0])
    exit.post_add (p0)

def mkdekker (n) :
    assert (n >= 2)
    net = ptnet.net.Net ()
    net.author = 'mkdekker.py'
    net.title = 'A variant of the Dekker\'s mutual exclusion algorithm'
    net.note = 'Instance with %d process. No turn management, process starvation is possible' % n

    # create flags
    flag = {}
    for i in range (n) :
        flag[i, 0] = ptnet.net.Place ('flag=0/%d' % i)
        flag[i, 1] = ptnet.net.Place ('flag=1/%d' % i)
        flag[i, 0].m0 = 1
        net.places.append (flag[i, 0])
        net.places.append (flag[i, 1])
        net.m0.add (flag[i, 0])

    for i in xrange (n) :
        mkproc (i, net, n, flag)
    net.write (sys.stdout, 'pep')
    # additionally you can activate PNML output at stderr
    # net.write (sys.stderr, 'pnml')

if __name__ == '__main__' :
    assert (len (sys.argv) == 2)
    n = int (sys.argv[1])
    mkdekker (n)

# vi:ts=4:sw=4:et:
