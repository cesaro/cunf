#!/usr/bin/env python

import sys
import ptnet

if __name__ == '__main__' :
    n = ptnet.net.Net (True)
    n.read (sys.stdin, 'pnml')
    n.plain2cont ()
    n.write (sys.stdout, 'pep')

# vi:ts=4:sw=4:et:
