#!/usr/bin/env python3

import sys
import ptnet.net

if __name__ == '__main__' :
    n = ptnet.net.Net (True)
    n.read (sys.stdin, 'pep')
    n.write (sys.stdout, 'grml')

# vi:ts=4:sw=4:et:
