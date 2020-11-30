#!/usr/bin/env python3

import sys
import ptnet.unfolding
import ptnet.mp

def output (k, v, fmt='%s') :
    sys.stderr.write (('%s\t' + fmt + '\n') % (k, v))

if __name__ == '__main__' :
    mp = ptnet.mp.Mprocess (True)
    mp.read (sys.stdin, fmt='mp')
    mp.write (sys.stdout, 'pep')

    output ('trans', len (mp.net.trans))
    output ('places', len (mp.net.places))
    output ('mp-evs', len (mp.mpevents))
    output ('mp-cond', len (mp.mpconds))
    output ('mp-cffs', mp.nr_cutoffs)

# vi:ts=4:sw=4:et:
