#!/usr/bin/env python3

import sys
import ptnet.unfolding
import ptnet.mp

def output (k, v, fmt='%s') :
    sys.stderr.write (('%s\t' + fmt + '\n') % (k, v))

if __name__ == '__main__' :
    u = ptnet.unfolding.Unfolding (True)
    u.read (sys.stdin, fmt='cuf')
    mp = u.merge ()
    mp.write (sys.stdout, 'mp')

    output ('places', len (u.net.places))
    output ('trans', len (u.net.trans))
    output ('events', len (u.events))
    output ('conds', len (u.conds))
    output ('blacks', u.nr_black)
    output ('grays', u.nr_gray)
    output ('mp-evs', len (mp.mpevents))
    output ('mp-cond', len (mp.mpconds))
    output ('mp-cffs', mp.nr_cutoffs)

# vi:ts=4:sw=4:et:
