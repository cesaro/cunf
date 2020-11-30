#!/usr/bin/env python3

'''
Generates a variant of the Peterson's mutual exclusion algorithm for two
processes.  The code actually produces a 'box' that comunicates trough signals
(places): begin, end, cs_begin, cs_end for each of the two processes involved.
That 'box' can be put into a larger c-net to model the algorithm within a
larger system.
'''

import sys
import ptnet

def mk_half_box (net, i, p, begin, end, cs_begin, cs_end,
        my_f0, my_f1, other_f0, other_of1, turn0, turn1) :

    # p should be 1 or 2
    # create all transitions
    raise_flag = ptnet.net.Transition ('raise_flag/p=%d/box=%d' % (p, i))
    turn_ok = ptnet.net.Transition ('turn_ok/p=%d/box=%d' % (p, i))
    turn_mine = ptnet.net.Transition ('turn_mine/p=%d/box=%d' % (p, i))
    enter_turn = ptnet.net.Transition ('enter_turn/p=%d/box=%d' % (p, i))
    enter_flag = ptnet.net.Transition ('enter_flag/p=%d/box=%d' % (p, i))
    exit = ptnet.net.Transition ('exit/p=%d/box=%d' % (p, i))
    for t in [raise_flag, turn_ok, turn_mine, enter_turn, enter_flag, exit] :
        net.trans.append (t)

    # and two private places
    s1 = ptnet.net.Place ('s1/p=%s/box=%d' % (p, i))
    s2 = ptnet.net.Place ('s2/p=%s/box=%d' % (p, i))
    for q in [s1, s2] :
        net.places.append (q)

    # i first raise my flag 
    raise_flag.pre_add (begin)
    raise_flag.pre_add (my_f0)
    raise_flag.post_add (my_f1)
    raise_flag.post_add (s1)

    # if the turn variable already points to me, i continue
    turn_ok.pre_add (s1)
    if p == 1 :
        turn_ok.cont_add (turn0)
    else :
        turn_ok.cont_add (turn1)
    turn_ok.post_add (s2)

    # if not, set the turn to point to me
    turn_mine.pre_add (s1)
    if p == 1 :
        turn_mine.pre_add (turn1)
        turn_mine.post_add (turn0)
    else :
        turn_mine.pre_add (turn0)
        turn_mine.post_add (turn1)
    turn_mine.post_add (s2)

    # if the turn is not mine, i enter the critical section
    enter_turn.pre_add (s2)
    if p == 1 :
        enter_turn.cont_add (turn1)
    else :
        enter_turn.cont_add (turn0)
    enter_turn.post_add (cs_begin)

    # if the other's flag is 0, i can also enter
    enter_flag.pre_add (s2)
    enter_flag.cont_add (other_f0)
    enter_flag.post_add (cs_begin)

    # to exit the critical section, i lower my flag
    exit.pre_add (cs_end)
    exit.pre_add (my_f1)
    exit.post_add (my_f0)
    exit.post_add (end)

def mk_box (net, i, begin1, cs_begin1, cs_end1, end1,
                    begin2, cs_begin2, cs_end2, end2) :

    # create flags for both processes
    f10 = ptnet.net.Place ('flag1=0/box=%d' % i)
    f11 = ptnet.net.Place ('flag1=1/box=%d' % i)
    f20 = ptnet.net.Place ('flag2=0/box=%d' % i)
    f21 = ptnet.net.Place ('flag2=1/box=%d' % i)

    # create a turn variable
    turn0 = ptnet.net.Place ('turn=0/box=%d' % i)
    turn1 = ptnet.net.Place ('turn=1/box=%d' % i)
    for q in [turn0, turn1, f10, f11, f20, f21] :
        net.places.append (q)

    # flags are initially 0 for both processes; the first has the turn
    net.m0[f10] = 1
    net.m0[f20] = 1
    net.m0[turn0] = 1

    # make both halves
    mk_half_box (net, i, 1, begin1, end1, cs_begin1, cs_end1,
            f10, f11, f20, f21, turn0, turn1)

    mk_half_box (net, i, 2, begin2, end2, cs_begin2, cs_end2,
            f20, f21, f10, f11, turn0, turn1)

def mk_peterson2 () :
    net = ptnet.net.Net ()
    net.author = 'mkpeterson.py'
    net.title = 'A variant of the Peterson mutual exclusion algorithm'
    net.note = ''

    begin1 = net.place_add ('begin1', 1)
    begin2 = net.place_add ('begin2', 1)
    cs1 = net.place_add ('cs1')
    cs2 = net.place_add ('cs2')

    mk_box (net, 0, begin1, cs1, cs1, begin1, begin2, cs2, cs2, begin2)
    #net.cont2plain ()
    net.write (sys.stdout, 'pep')

if __name__ == '__main__' :
    assert (len (sys.argv) == 2)
    n = int (sys.argv[1])
    mk_peterson2 ()

# vi:ts=4:sw=4:et:
