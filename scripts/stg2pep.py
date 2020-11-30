#!/usr/bin/env python3

import sys
import ptnet

if __name__ == '__main__' :
    u = ptnet.net.Net (True)
    u.read (sys.stdin, 'stg')
    u.write (sys.stdout, 'pep')

# vi:ts=4:sw=4:et:
