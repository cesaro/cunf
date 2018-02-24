#!/usr/bin/env python

import sys
import ptnet.net
import ptnet.unfolding

def main1 () :
    n = ptnet.net.Net (True)

    # path to the .pt file to open
    f = open ('./examples/plain/small-stubborn2/only_hl.pt', 'r')
    n.read (f, 'pt1')

    # sequence of 'strings' with the names of the transitions to execute
    run = ['T110', 'T1', 'aux12_init__onl_onr', 'aux12_r__go',
    'aux1_init__onl_onr', 'aux1_l__go', 'aux12_l__wait', 'aux13_init__onl_onr',
    'aux13_l__go', 'T2', 'aux24_init__onl_onr', 'aux24_r__go',
    'aux24_l__wait', 'aux25_init__onl_onr', 'aux25_l__go', 'aux25_r__wait',
    'aux13_r__wait', 'aux1_r__wait']
    
    # m is a marking (a state), initiall the initial marking
    m = n.m0
    i = 0
    for t in run :
        print '========================================= i', i
        print 'm', m
        print 't', t

        # en is the ste of enabled transitions
        en = n.enabled (m)
        print 'enabled', en

        en2 = [tt for tt in en if tt.name == t]
        #print 'len', len (en2)
        #for x in en : print 'xxx', t, x.name, t == x.name
        assert len (en2) == 1
        print 'en2', en2[0]

        # execute the transition t, updating the marking m
        n.fire ([en2[0]], m)
        i += 1

    print 'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx'
    print 'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx'
    print 'final marking', m
    print 'it enables', n.enabled (m)

def main2 () :
    u = ptnet.unfolding.Unfolding (True)
    f = open ('./examples/plain/small-stubborn2-ll_net/only_hl.unf.cuf', 'r')
    u.read (f)

    print u.events

    # estado cuando hemos terminado de hablar, viernes
    # - la pila y la configuracion son iguales
    # - v es una configuracion
    # - T38 (evento 54) no esta activo en v \ {54}

if __name__ == '__main__' :
    main1 ()

# vi:ts=4:sw=4:et:
