#!/usr/bin/env python

import sys
import ptnet.net

if __name__ == '__main__' :
    n = ptnet.net.Net (True)
    n.read (sys.stdin, 'pt1')
    n.stubbornify ()
    n.cont2plain ()
    n.write (sys.stdout, 'pt1')

# vi:ts=4:sw=4:et:
