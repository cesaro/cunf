#!/usr/bin/env python

'''
Evaluation on two different fomulas of different at-most-one (AMO) encodings
'''

import sys
import ptnet
import math

class Formula (object) :
    def __init__ (self) :
        self.varmap = {}
        self.clsset = set ()

    def var (self, obj) :
        if not obj in self.varmap :
            self.varmap[obj] = len (self.varmap) + 1
        return self.varmap[obj]

    def add (self, cls) :
        self.clsset.add (frozenset (cls))

    def amo_standard (self, l) :
        # for n >= 0 items, produces (n^2-n)/2 clauses, no new variables
        for i in xrange (len (l)) :
            for j in xrange (i + 1, len (l)) :
                self.add ([-l[i], -l[j]])

    def amo_2tree (self, l) :
        # for n >= 3 items, produces n-1 new vars and 3n-5 clauses
        if len (l) <= 1 : return
        if len (l) == 2 :
            self.add ([-l[0], -l[1]])
            return

        l2 = []
        if len (l) & 1 : l2.append (l.pop ())
        for i in xrange (0, len (l), 2) :
            v1 = l[i]
            v2 = l[i + 1]
            v = self.var (('amo_2tree', v1, v2))

            self.add ([-v1, -v2])
            self.add ([-v1, v])
            self.add ([-v2, v])
            l2.append (v)
        self.amo_2tree (l2)

    def amo_ktree (self, l, k) :
        if len (l) <= 1 : return
        assert (k >= 2)
        if len (l) <= k :
            self.amo_standard (l)
            return

        l2 = l[: len (l) % k]
        del l[: len (l) % k]
        for i in xrange (0, len (l), k) :
            l3 = l[i:i + k]
            print 'c i', i, 'k', k, 'l3', l3, 'l', l
            v = self.var (('amo_ktree', frozenset (l3)))
            for vi in l3 : self.add ([-vi, v])
            self.amo_standard (l3)
            l2.append (v)
        self.amo_ktree (l2, k)

    def amo_seq (self, l) :
        # for n >= 3, produces n-2 new vars and 3n-5 clauses
        if len (l) <= 1 : return
        self.add ([-l[0], -l[1]])
        if len (l) == 2 : return
        ref = frozenset (l)
        self.add ([-l[0], self.var (('amo_seq', ref, l[1]))])
        va = self.var (('amo_seq', ref, l[-2]))
        self.add ([-l[-2], va])
        self.add ([-va, -l[-1]])
        if len (l) == 3 : return

        for i in xrange (1, len (l) - 2) :
            va = self.var (('amo_seq', ref, l[i]))
            va1 = self.var (('amo_seq', ref, l[i + 1]))
            self.add ([-l[i], va])
            self.add ([-va, va1])
            self.add ([-va, -l[i + 1]])

    def amo_bin (self, l) :
        # for n >= 2, generates ceil(log n) new vars and n*ceil(log n) clauses at most
        if len (l) <= 1 : return
        k = int (math.ceil (math.log (len (l), 2)))
        z = (1 << k) - len (l)

        i = 0
        fs = frozenset (l)
        for v in l :
            f = 1 if z >= 1 else 0
            z -= 1
            for j in xrange (f, k) :
                p = self.var (('amo_bin', fs, j))
                if (1 << j) & i == 0 : p = -p
                self.add ([-v, p])
            i += 1 + f

    def amo_kprod (self, l, k) :
        assert (k >= 2)
        if len (l) <= 1 : return
        if len (l) == 2 :
            self.add ([-l[0], -l[1]])
            return

        print 'c k', k, 'l', l
        ref = frozenset (l)
        # there are better ways to implement this comparison, k might be large!
        if (1 << (k - 1)) >= len (l) :
            k = int (math.ceil (math.log (len (l), 2)))
            print 'c new k', k
        b = int (math.ceil (len (l) ** (1.0 / k)))
        print 'c b', b
        assert ((b ** k) >= len (l))

        t = [0 for i in xrange (k)]
        for v in l :
            print 'c v', v, 'assigned to', t
            for i in xrange (k) :
                vij = self.var (('amo_kprod', ref, i, t[i]))
                print 'c %d -> %d' % (v, vij)
                self.add ([-v, vij])
            for i in xrange (k) :
                t[i] += 1
                if t[i] < b : break
                t[i] = 0
        for i in xrange (k - 1) :
            l2 = [self.var (('amo_kprod', ref, i, j)) for j in xrange (b)]
            print 'c amo dim', i, 'l2', l2
            self.amo_kprod (l2, k)
        i = t[k - 1] + 1 if t[k - 1] != 0 else b
        l2 = [self.var (('amo_kprod', ref, k - 1, j)) for j in xrange (i)]
        print 'c amo dim', k-1, 'l2', l2
        self.amo_kprod (l2, k)

    def __repr__ (self) :
        s = ''
        for k in self.varmap :
            s += 'c "%s" => %d\n' % (k, self.varmap[k])
        s += '\np cnf %d %d\n' % (len (self.varmap), len (self.clsset))
        for c in self.clsset :
            for v in c : s += str (v) + ' '
            s += '0\n'
        return s

    def __str__ (self) :
        return self.__repr__ ()

def lin (f, n) :
    l = []
    for x in range (n) :
        l.append (f.var (x))
    _lin (f, l)

def main () :
    u = ptnet.unfolding.Unfolding ()
    #f = open ('d.unf.cuf', 'r')
    u.read (sys.stdin)
    u.stats (sys.stdout)

def main1 () :
    ptnet.unfolding.test2 ()

def main2 () :
    s = sys.argv[1]
    n = int (sys.argv[2])
    f = Formula ()

    # at most one
    l = []
    for x in range (n) : l.append (f.var (x))
    if s == 'standard' : 
        f.amo_standard (l)
    elif s == 'seq' :
        f.amo_seq (l)
    elif s == 'bin' :
        f.amo_bin (l)
    elif 'bintree' == s :
        f.amo_2tree (l)
    elif '-tree' in s :
        (k, bar, tree) = s.partition ('-')
        assert (bar == '-' and tree == 'tree')
        k = int (k)
        f.amo_ktree (l, k)
    elif '-prod' in s :
        (k, bar, prod) = s.partition ('-')
        assert (bar == '-' and prod == 'prod')
        k = int (k)
        f.amo_kprod (l, k)
    else :
        assert (False)

    unsat_al2 (f, n)
    print f
    print >> sys.stderr, 'nnnnnnnnnnnnnnnn', n

def unsat_p1p2 (f, n) :
    # at least one
    f.add ([f.var (x) for x in xrange (n)])

    # none of them can actually happen
    for x in xrange (n) :
        v1 = f.var ((x, 'p1'))
        v2 = f.var ((x, 'p2'))
        v = f.var (x)

        f.add ([-v, v1])
        f.add ([-v, v2])
        f.add ([-v1, -v2])

def unsat_al2 (f, n) :
    f.add ([f.var (x) for x in xrange (n / 2)])
    f.add ([f.var (x) for x in xrange (n / 2, n)])

def main3 () :
    f = Formula ()
    v1 = f.var (None)
    v2 = f.var (1)
    f.add ([-v1, v2])
    f.add ([v1, v2])
    print f

if __name__ == '__main__' :
    main2 ()

# vi:ts=4:sw=4:et:
