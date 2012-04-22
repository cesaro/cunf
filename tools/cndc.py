#!/usr/bin/env python

'''
The Contextual Net Deadlock Checker (cndc).

Copyright (C) 2012  Cesar Rodriguez <cesar.rodriguez@lsv.ens-cachan.fr>
Laboratoire de Specification et Verification (LSV), ENS Cachan, France

This program comes with ABSOLUTELY NO WARRANTY.  This is free software, and
you are welcome to redistribute it under certain conditions.  You should have
received a copy of the GNU General Public License along with this program.  If
not, see <http://www.gnu.org/licenses/>.

Usage: cndc.py [OPTIONS] PATH

Where PATH is the filepath to a .cuf file, or '-' for standard input, and
OPTIONS is a list of zero or more optimization identifiers chosen from the
following:

  k-tree  (where k >= 2)  Use k-trees to implement the at-most-one
          constraints in the encoding of symmetric conflicts.
          If none of the options 'seq', 'log' or 'pair' is present, 4-tree
          will be used
  seq     Use the sequential encoding of [Sinz 05] for symmetric conflicts.
  log     Use the logaritmic encoding of [Frisch 05] for symmetric conflicts.
  pair    Use pairwise encoding for symmetric conflicts.

  stb     Use elimination of stubborn events.
  sub     Reduce the symmetric and disabled constraints to certain maximal
          sets.

  nocy    Do not produce constraints to check for cycles in the asymmetric
          conflict relation.
  bin     Generate acyclicity constraints using ranks with binary encoding.
          See [Codish 09].  If no none of the options 'nocy', 'trans' or
          'unary' is present, 'bin' will be used.
  unary   Generate acyclicity constraints using ranks with unary encoding.
  trans   Generate acyclicity constraints encoding transitive closure.
  sccred  Reduce the SCCs before generating the acyclicity constraints.
'''

import sys
import time
import math
import networkx

class Event (object) :
    def __init__ (self, nr, l="", pre=set(), post=set(), cont=set(),
            white=True, gray=False) :
        self.nr = nr
        self.label = l
        self.pre = pre
        self.post = post
        self.cont = cont
        self.isblack = not white and not gray
        self.iswhite = white
        self.isgray = gray
        self.mark = 0
        self.count = 0
        #db ('Event', nr, l, pre, post, cont, 'white', white, 'gray', gray)

        for c in pre : c.post.add (self)
        for c in post : c.pre = self
        for c in cont : c.cont.add (self)

    def __repr__ (self) :
        if self.isblack :
            s = '*'
        elif self.isgray :
            s = '+'
        else :
            s = ''

        return '%se%d:%s' % (s, self.nr, self.label)

    def __str__ (self) :
        if self.isblack :
            s = '*'
        elif self.isgray :
            s = '+'
        else :
            s = ''
        return "%se%d:%s Pre %s;  Cont %s;  Post %s" \
                % (s, self.nr, self.label, self.pre, self.cont, self.post)

class Condition (object) :
    def __init__ (self, nr, l="", pre=None, post=set(), cont=set()) :
        self.nr = nr
        self.label = l
        self.pre = pre
        self.post = post
        self.cont = cont
        self.mark = 0
        self.count = 0

        if (pre) : pre.post.add (self)
        for e in post : e.pre.add (self)
        for e in cont : e.cont.add (self)

    def __repr__ (self) :
        return 'c%d:%s' % (self.nr, self.label)

    def __str__ (self) :
        return 'c%d:%s Pre %s;  Cont %s;  Post %s' \
                % (self.nr, self.label, repr (self.pre), self.cont, self.post)

class Unfolding (object) :

    def __init__ (self, sanity_check=True) :
        self.conds = [None]
        self.events = [None]
        self.m0 = set ()
        self.nr_black = 0
        self.nr_gray = 0
        self.sanity_check = sanity_check
        self.mark = 1

    def __is_event_id (self, i) :
        return 0 <= i < len (self.events)

    def __is_cond_id (self, i) :
        return 1 <= i < len (self.conds)

    def __sane_event_id (self, i) :
        if (self.__is_event_id (i)) : return
        raise Exception, '%s: invalid event nr' % i
        
    def __sane_cond_id (self, i) :
        if (self.__is_cond_id (i)) : return
        raise Exception, '%s: invalid condition nr' % i

    def add_cond (self, l, pre=0, post=[], cont=[]) :
        # validate event identifiers
        if self.sanity_check :
            self.__sane_event_id (pre)
            for i in post : self.__sane_event_id (i)
            for i in cont : self.__sane_event_id (i)

        # map to object references
        po = set(self.events[i] for i in post)
        co = set(self.events[i] for i in cont)
        #db ('new cond', l, pre, post, cont, 'initial', pre == 0)

        # create the new condition and register it
        c = Condition (len (self.conds), l, self.events[pre], po, co)
        self.conds.append (c)
        if (pre == 0) : self.m0.add (c)
        return c

    def add_event (self, l, pre=[], post=[], cont=[], white=True, gray=False) :
        # validate condition identifiers
        if self.sanity_check :
            for i in pre : self.__sane_cond_id (i)
            for i in post : self.__sane_cond_id (i)
            for i in cont : self.__sane_cond_id (i)

        # map to object references
        pr = set(self.conds[i] for i in pre)
        po = set(self.conds[i] for i in post)
        co = set(self.conds[i] for i in cont)

        # create the new event and register it
        e = Event (len (self.events), l, pr, po, co, white, gray)
        self.events.append (e)
        if gray : self.nr_gray += 1
        if not white and not gray : self.nr_black += 1
        return e

    def __cuf2unf_readstr (self, f, m) :
        s = ""
        for i in xrange (0, m) :
            t = f.read (1)
            if (len (t) == 0) : break
            if (t == '\x00') : return s
            s += t
        raise Exception, 'Corrupted CUF file'

    def __cuf2unf_readint (self, f) :
        s = f.read (4)
        if (len (s) != 4) : raise Exception, 'Corrupted CUF file'
        r = (ord (s[0]) << 24) + (ord (s[1]) << 16)
        r += (ord (s[2]) << 8) + ord (s[3])
        return r

    def read (self, f, fmt='cuf') :
        if fmt != 'cuf' : raise Exception, "'%s': unknown input format" % fmt
        mag = self.__cuf2unf_readint (f)
        if mag != 0x43554602 : raise Exception, "'%s': not a CUF02 file" % fmt

        # read first four fields
        nrc = self.__cuf2unf_readint (f)
        nre = self.__cuf2unf_readint (f)
        nrw = self.__cuf2unf_readint (f)
        nrg = self.__cuf2unf_readint (f)
        m = 1 + self.__cuf2unf_readint (f)
        #r, nrc, nre, nrf, m

        # read nre event labels
        for i in xrange (nrw) :
            self.add_event (self.__cuf2unf_readstr (f, m))
        for i in xrange (nrg) :
            l = self.__cuf2unf_readstr (f, m)
            self.add_event (l, white=False, gray=True)
        for i in xrange (nre - nrg - nrw) :
            l = self.__cuf2unf_readstr (f, m)
            self.add_event (l, white=False, gray=False)

        # read condition labels, flow and context relations
        for i in xrange (0, nrc) :
            l = self.__cuf2unf_readstr (f, m)
            pre = self.__cuf2unf_readint (f)
            post = []
            cont = []
            while True :
                e = self.__cuf2unf_readint (f)
                if (not e) : break
                post.append (e)
            while True :
                e = self.__cuf2unf_readint (f)
                if (not e) : break
                cont.append (e)
#            db (l, pre, post, cont)
            self.add_cond (l, pre, post, cont)
        if self.sanity_check :
            for e in self.events[1:] :
                if not e.pre and not e.cont :
                    raise Exception, 'Event e%d has empty preset+context' % e.nr

    def asym_graph (self, symm=True, s=None, cutoffs=False) :
        # build the asymmetric conflict relaton
        g = networkx.DiGraph ()
        u = self.events[1:] if s == None else s
        for e in u :
            if not cutoffs and e.isblack : continue
            for c in e.pre :
                if c.pre : g.add_edge (c.pre, e, color=1)
                for ep in c.cont : self.__red_edge (g, ep, e, 2)
                if symm :
                    for ep in c.post :
                        if ep != e and (cutoffs or not ep.isblack) :
                            g.add_edge (ep, e, color=1)
                            g.add_edge (e, ep, color=1)
            for c in e.cont :
                if c.pre and (cutoffs or not c.pre.isblack):
                    g.add_edge (c.pre, e, color=1)

        if s != None : g = g.subgraph (s)
#        opt = ''
#        if symm : opt += 'with symm. confl. '
#        if s != None : opt += 'restricted to %d events ' % len (s)
#        if cutoffs : opt += 'with cutoffs (black)!'
#        db ('nodes', len (g), 'edges', len (g.edges()), opt)

        return g

    def __red_edge (self, g, u, v, c) :
        g.add_edge (u, v)
        if not 'color' in g[u][v] or g[u][v]['color'] == 2 :
            g[u][v]['color'] = c

class Cnf (object) :
    def __init__ (self) :
        self.varmap = {}
        self.clsset = set ()

    def var (self, obj) :
        if not obj in self.varmap :
            self.varmap[obj] = len (self.varmap) + 1
        return self.varmap[obj]

    def add (self, cls) :
        self.clsset.add (frozenset (cls))

    def amo_pairwise (self, l) :
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
            self.amo_pairwise (l)
            return

        l2 = l[: len (l) % k]
        del l[: len (l) % k]
        for i in xrange (0, len (l), k) :
            l3 = l[i:i + k]
#            db ('c i', i, 'k', k, 'l3', l3, 'l', l)
            v = self.var (('amo_ktree', frozenset (l3)))
            for vi in l3 : self.add ([-vi, v])
            self.amo_pairwise (l3)
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

        db ('c k', k, 'l', l)
        ref = frozenset (l)
        # there are better ways to implement this comparison, k might be large!
        if (1 << (k - 1)) >= len (l) :
            k = int (math.ceil (math.log (len (l), 2)))
            db ('c new k', k)
        b = int (math.ceil (len (l) ** (1.0 / k)))
        db ('c b', b)
        assert ((b ** k) >= len (l))

        t = [0 for i in xrange (k)]
        for v in l :
            db ('c v', v, 'assigned to', t)
            for i in xrange (k) :
                vij = self.var (('amo_kprod', ref, i, t[i]))
                db ('c %d -> %d' % (v, vij))
                self.add ([-v, vij])
            for i in xrange (k) :
                t[i] += 1
                if t[i] < b : break
                t[i] = 0
        for i in xrange (k - 1) :
            l2 = [self.var (('amo_kprod', ref, i, j)) for j in xrange (b)]
            db ('c amo dim', i, 'l2', l2)
            self.amo_kprod (l2, k)
        i = t[k - 1] + 1 if t[k - 1] != 0 else b
        l2 = [self.var (('amo_kprod', ref, k - 1, j)) for j in xrange (i)]
        db ('c amo dim', k-1, 'l2', l2)
        self.amo_kprod (l2, k)

    def write (self, f) :
        f.write ('p cnf %d %d\n' % (len (self.varmap), len (self.clsset)))
        for c in self.clsset :
            s = ''
            for v in c : s += str (v) + ' '
            s += '0\n'
            f.write (s)

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

class Cndc (object) :
    def __init__ (self, opt) :
        self.opt = {}

        self.opt['path'] = opt['path']
        self.opt['verbose'] = 'verbose' in opt and opt['verbose']
        self.opt['assert'] = 'assert' in opt and opt['assert']

        self.opt['tree'] = opt['tree'] if 'tree' in opt else 0
        self.opt['seq'] = 'seq' in opt and opt['seq']
        self.opt['log'] = 'log' in opt and opt['log']
        self.opt['pair'] = 'pair' in opt and opt['pair']

        self.opt['nocy'] = 'nocy' in opt and opt['nocy']
        self.opt['bin'] = 'bin' in opt and opt['bin']
        self.opt['unary'] = 'unary' in opt and opt['unary']
        self.opt['trans'] = 'trans' in opt and opt['trans']

        self.opt['sccred'] = 'sccred' in opt and opt['sccred']
        self.opt['stb'] = 'stb' in opt and opt['stb']
        self.opt['sub'] = 'sub' in opt and opt['sub']

        # validate options
        n = 0
        if self.opt['tree'] != 0 : n += 1
        if self.opt['seq'] : n += 1
        if self.opt['log'] : n += 1
        if self.opt['pair'] : n += 1
        if n == 0 : self.opt['tree'] = 4
        if n > 1 : raise Exception, 'At most one of k-tree, seq, log or pair!'
        if self.opt['tree'] == 1 : raise Exception, 'Use k > 1 for k-trees!'

        n = 0
        if self.opt['nocy'] : n += 1
        if self.opt['bin'] : n += 1
        if self.opt['unary'] : n += 1
        if self.opt['trans'] : n += 1
        if n == 0 : self.opt['bin'] = True
        if n > 1 : raise Exception, 'At most one of nocy, bin, unary or trans!'
#        db (self.opt)

        # declare additional fields
        self.phi = Cnf ()
        self.u = Unfolding (self.opt['assert'])
        self.result = {}
        self.stubborn = {}
        self.pred = {}

        # load the model
        try :
            if opt['path'] == '-' :
                f = sys.stdin
            else :
                f = open (opt['path'], 'r')
            self.u.read (f, fmt='cuf')
            f.close ()
        except IOError as (e, m) :
            raise Exception, "'%s': %s" % (opt['path'], m)
        except Exception, e:
            raise Exception, "'%s': %s" % (opt['path'], e)

        s = ''
        for k in self.opt :
            if k != 'path' and k != 'tree' and self.opt[k] : s += k + ' '
        if self.opt['tree'] != 0 : s += '%d-tree' % self.opt['tree']
        self.result['opts'] = s

    def deadlock (self, f) :

        # nothing to do if there is zero cutoffs
        if self.u.nr_black == 0 :
            f.write ('c zero cutoffs!\np cnf 1 1\n1 0\n')
            return

        # causal closure
        self.__causal ()
        self.result['evvars'] = len (self.phi.varmap)

        # symmetric conflicts
        self.__sym ()

        # asymmetric conflicts
        self.__asym ()

        # marking and disabled
        self.__dis ()

        # output in DIMACS CNF
        self.phi.write (f)

        self.result['vars'] = len (self.phi.varmap)
        self.result['clauses'] = len (self.phi.clsset)

    def __causal (self) :
        # a configuration is causally closed ...
        for e in self.u.events[1:] :
            if e.isblack or self.__is_stubborn (e) : continue
            s = self.__pred (e)
            atm = -self.phi.var (e)
            if not s : continue
            for ep in s :
                self.phi.add ([atm, self.phi.var (ep)])

    def __sym_ismaximal (self, c) :
        # c has at least two non-black events
        if not self.opt['sub'] : return True
        m = self.u.mark
        self.u.mark += 1
        l = [e for e in c.post if not e.isblack]
        n = len (l)
        for e in l[1:] :
            for cp in e.pre :
                if cp == c : continue
                if cp.mark != m :
                    cp.mark = m
                    cp.count = 1
                else :
                    cp.count += 1
        for cp in l[0].pre :
            if cp == c or cp.mark != m : continue
            cp.count += 1
            if cp.count == n :
                np = len ([e for e in cp.post if not e.isblack])
#                if n < np or cp.nr > c.nr : db ('symm_not_max', c, cp)
                if n < np or cp.nr > c.nr : return False
        return True

    def __sym (self) :
        # ... and has no symmetric conflict
        for c in self.u.conds[1:] :
            l = [e for e in c.post if not e.isblack]
            if len (l) < 2 : continue
            if not self.__sym_ismaximal (c) : continue
            ll = [self.phi.var (e) for e in l]
            if self.opt['tree'] > 1 :
                self.phi.amo_ktree (ll, self.opt['tree'])
            elif self.opt['seq'] :
                self.phi.amo_seq (ll)
            elif self.opt['log'] :
                self.phi.amo_bin (ll)
            elif self.opt['pair'] :
                self.phi.amo_pairwise (ll)
            else :
                raise Exception, 'sym internal error'

    def __asym (self) :
        if self.opt['nocy'] : return

        # generate asymmetric conflict graph (without sym. conflicts)
        g = self.u.asym_graph (False)

        # search for sccs
        sccs = networkx.algorithms.strongly_connected_components (g)
        sccs = [x for x in sccs if len (x) >= 2]

        sizes = []
        for x in sccs :
            h = g.subgraph (x)
            m = h.size ()
            if self.opt['sccred'] :
                h = self.__reduce (h)
            sizes.append ((len (x), m, len (h), h.size ()))
            self.__asym_scc (h)
        self.result['sccs'] = repr (sizes)

    def __asym_scc (self, g) :
        if self.opt['trans'] :
            self.__asym_scc_trans (g)
            return

        k = int (math.ceil (math.log (len (g), 2)))
#        k = 1
#        db ('k', k);
        for e in g :
            for ep in g[e] :
                if self.opt['bin'] :
                    self.__asym_scc_bin_ord (e, ep, k)
                elif self.opt['unary'] :
                    self.__asym_scc_una_ord (e, ep, len (g))
                else :
                    raise Exception, 'asym internal error'
        if self.opt['unary'] and len (g[e]) :
            self.__asym_scc_una (e, len (g))

    def __asym_scc_bin_ord (self, a, b, k) :
        # a > b
#        db (repr (a), repr (b), k)
        ai = self.phi.var ((a, 0))
        bi = self.phi.var ((b, 0))
        ci1 = self.phi.var ((a, b, 0))

        self.phi.add ([ai, -ci1])
        self.phi.add ([-ai, -bi, -ci1])
#        self.phi.add ([-ai, bi, ci]) # optim.

        for i in xrange (1, k) :
            ai = self.phi.var ((a, i))
            bi = self.phi.var ((b, i))
            ci = self.phi.var ((a, b, i))

            self.phi.add ([ai, -bi, -ci])
            self.phi.add ([-ai, -bi, -ci1, ci])
            self.phi.add ([-ai, -bi, ci1, -ci])
            self.phi.add ([ai, bi, -ci1, ci])
            self.phi.add ([ai, bi, ci1, -ci])
#            self.phi.add ([-ai, bi, ci]) # optim.
            ci1 = ci

        if self.__is_stubborn (a) :
            l = [-self.phi.var (e) for e in self.__pred (a)]
        else :
            l = [-self.phi.var (a)]

        if self.__is_stubborn (b) :
            l += [-self.phi.var (e) for e in self.__pred (b)]
        else :
            l += [-self.phi.var (b)]
        l.append (self.phi.var ((a, b, k - 1)))
        self.phi.add (l)

    def __asym_scc_trans (self, g) :
        for e in g :
            if self.__is_stubborn (e) :
                l = [-self.phi.var (ep) for ep in self.__pred (e)]
            else :
                l = [-self.phi.var (e)]

            for ep in g[e] :
                if self.__is_stubborn (ep) :
                    l2 = [-self.phi.var (epp) for epp in self.__pred (ep)]
                else :
                    l2 = [-self.phi.var (ep)]
                self.phi.add (l + l2 + [self.phi.var ((e, ep))])

        for e in g :
            for e1 in g :
                if e == e1 : continue
                atm = - self.phi.var ((e, e1))
                atm1 = - self.phi.var ((e1, e))
                self.phi.add ([atm, atm1])

        for e in g :
            for e1 in g :
                if e == e1 : continue
                atm = - self.phi.var ((e, e1))
                for e2 in g[e1] :
                    if e == e2 or e2 in g[e] : continue
                    atm1 = -self.phi.var ((e1, e2))
                    atm2 = self.phi.var ((e, e2))
                    self.phi.add ([atm, atm1, atm2])

    def __asym_scc_una (self, a, k) :
        aim1 = self.phi.var ((a, 0))
#        db (aim1, 'is', (a, 0))
        for i in xrange (1, k) :
            ai = self.phi.var ((a, i))
            self.phi.add ([-ai, aim1])
#            db (ai, 'is', (a, i))
#            db ('clause', [-ai, aim1])
            aim1 = ai

    def __asym_scc_una_ord (self, a, b, k) :
        # a > b
        if self.__is_stubborn (a) :
            l = [-self.phi.var (e) for e in self.__pred (a)]
        else :
            l = [-self.phi.var (a)]

        if self.__is_stubborn (b) :
            l += [-self.phi.var (e) for e in self.__pred (b)]
        else :
            l += [-self.phi.var (b)]
        c = self.phi.var ((a, b))
        l.append (c)
        self.phi.add (l)

        ai = self.phi.var ((a, 0))
        bi = self.phi.var ((b, k - 1))
        self.phi.add ([-c, ai])
        self.phi.add ([-c, -bi])
#        db (ai, 'is', (a, 0))
#        db (bi, 'is', (b, k-1))
#        db ('clause', [-c, ai])
#        db ('clause', [-c, -bi])
        for i in xrange (k - 1) :
            bi = self.phi.var ((b, i))
            ai = self.phi.var ((a, i + 1))
            self.phi.add ([-c, -bi, ai])
#            db (bi, 'is', (b, i))
#            db (ai, 'is', (a, i+1))
#            db ('clause', [-c, -bi, ai])

    def __mark (self, c) :
        if c.pre and c.pre.isblack : return
        l = [self.phi.var (c.label)]
        if c.pre :
            if self.__is_stubborn (c.pre) :
                for e in self.__pred (c.pre) :
                    l.append (- self.phi.var (e))
            else :
                l.append (- self.phi.var (c.pre))
        l2 = [e for e in c.post if not e.isblack]
        if len (l2) == 1 and self.__is_stubborn (l2[0]) :
            for e in self.__pred (l2[0]) :
                self.phi.add (l + [self.phi.var (e)])
            return
        for e in l2 : l.append (self.phi.var (e))
        self.phi.add (l)

    def __dis (self) :
        tmp = set ()
        minimal = []
        tseen = set ()
        for e in self.u.events[1:] :
            if e.label in tseen : continue
            tseen.add (e.label)
            new = frozenset (c.label for c in e.pre | e.cont)
            if self.opt['sub'] :
                tmp.clear ()
                for elem in minimal :
#                    if new >= elem : db ('discarding', new);
                    if new >= elem : new = None; break
                    if new < elem : tmp.add (elem)
                for elem in tmp : minimal.remove (elem)
#                for elem in tmp : db ('removing', elem)
            if new : minimal.append (new)
#            if new : db ('taking', new)
        tmp.clear ()

        pneed = set ()
        for elem in minimal :
            self.phi.add ([-self.phi.var (p) for p in elem])
            pneed |= elem

        for c in self.u.conds[1:] :
            if c.label in pneed :
                self.__mark (c)

    def __reduce (self, g) :
        again = True
        while again :
            again = False
            for v in g :
                i = g.in_degree (v)
                o = g.out_degree (v)
                if i == 1 :
                    used = self.__red_1 (g, v)
                    if not used : used = self.__red_5 (g, v)
                    if not used : used = self.__red_7 (g, v)
                    if not used : used = self.__red_9 (g, v)
                elif o == 1 :
                    used = self.__red_6 (g, v)
                elif i == 2 and o == 2 :
                    used = self.__red_8 (g, v)
                else :
                    used = False
                again = again or used
        return g

    def __red_edge (self, g, u, v, c) :
        g.add_edge (u, v)
        if not 'color' in g[u][v] or g[u][v]['color'] == 2 :
            g[u][v]['color'] = c

    def __red_1 (self, g, v) :
        # assume that indeg (v) == 1
        # check that pre(v) and post(v) is green

        u = g.predecessors (v)[0]
        if g[u][v]['color'] != 1 : return False
        for w in g[v] :
            if g[v][w]['color'] != 1 : return False
        for w in g[v] : g.add_edge (u, w, color=1)
        g.remove_node (v)
        return True

    def __red_5 (self, g, v) :
        # assume that indeg (v) == 1
        # check that pre(v) is red and post(v) is green

        u = g.predecessors (v)[0]
        if g[u][v]['color'] != 2 : return False
        for w in g[v] :
            if g[v][w]['color'] != 1 : return False
        for w in g[v] : self.__red_edge (g, u, w, 2)
        g.remove_node (v)
        return True

    def __red_6 (self, g, v) :
        # assume that outdeg (v) == 1
        # check that post(v) is green

        u = g.successors (v)[0]
        if g[v][u]['color'] != 1 : return False
        for w in g.predecessors (v) :
            self.__red_edge (g, w, u, g[w][v]['color'])
        g.remove_node (v)
        return True

    def __red_7 (self, g, v) :
        # assume that indeg (v) == 1 and that __red_1 is not applicable
        # check that pre(v) is green

        u = g.predecessors (v)[0]
        if g[u][v]['color'] != 1 : return False
        found = False
        l = []
        for w in g[v] :
            if g[v][w]['color'] == 1 :
                l.append (w)
                g.add_edge (u, w, color=1)
        for w in l : g.remove_edge (v, w)
        return len (l) != 0

    def __red_8 (self, g, v) :
        # assume that indeg (v) == 2 and outdeg (v) == 2
        # check that there is 1 green, 1 red incoming and 2 green outgoing

        u1 = g.predecessors (v)[0]
        u2 = g.predecessors (v)[1]
        c = 2 if g[u1][v]['color'] == 1 else 1
        if g[u2][v]['color'] != c : return False
        w1 = g.successors (v)[0]
        w2 = g.successors (v)[1]
        if g[v][w1]['color'] != 1 or g[v][w2]['color'] != 1 : return False

        self.__red_edge (g, u1, w1, g[u1][v]['color'])
        self.__red_edge (g, u1, w2, g[u1][v]['color'])
        self.__red_edge (g, u2, w1, c)
        self.__red_edge (g, u2, w2, c)
        g.remove_node (v)
        return True

    def __red_9 (self, g, v) :
        # assume that indeg (v) == 1 and that __red_5 is not applicable
        # check that pre(v) is red

        u = g.predecessors (v)[0]
        if g[u][v]['color'] != 2 : return False
        l = []
        for w in g[v] :
            if g[v][w]['color'] == 1 :
                l.append (w)
                self.__red_edge (g, u, w, 2)
        for w in l : g.remove_edge (v, w)
        return len (l) != 0

    def __is_stubborn (self, e) :
#       st3: post (pre (e) U cont (e)) without blacks subset of {e}
        if not self.opt['stb'] : return False
        if e in self.stubborn : return self.stubborn[e]

        b = True
        for c in e.pre | e.cont :
            for ep in c.post :
                if not ep.isblack and ep != e :
                    b = False
                    break

        self.stubborn[e] = b
        return b

    def __pred (self, e) :
        if e in self.pred : return self.pred[e]
        s = set ();
        for c in e.pre | e.cont :
            if c.pre == None : continue
            if self.__is_stubborn (c.pre) :
                s |= self.__pred (c.pre)
            else :
                s.add (c.pre)
        self.pred[e] = s
        return self.pred[e]

def db (*msg) :
    s = ' '.join (str(x) for x in msg)
    sys.stderr.write ('debug: ' + s + '\n')

def usage () :
    sys.stderr.write (__doc__)
    sys.exit (1)

def parse () :
    opts = {}
    opts['seq'] = False
    opts['log'] = False
    opts['pair'] = False
    opts['stb'] = False
    opts['sub'] = False
    opts['nocy'] = False
    opts['bin'] = False
    opts['unary'] = False
    opts['trans'] = False
    opts['sccred'] = False
    path = None

    # at least one argument, the input file
    if len (sys.argv) < 2 : usage ()

    # fill the arguments with the expected options
    for arg in sys.argv[1:] :
        if arg in opts :
            opts[arg] = True
        elif '-tree' in arg :
            (k, sep, s) = arg.partition ('-tree')
            try :
                if len (s) : raise Exception
                opts['tree'] = int (k)
            except Exception, e :
                if path != None :
                    raise Exception, "'%s': unknown option" % arg
                path = arg
        else :
            if path != None :
                raise Exception, "'%s': unknown option" % arg
            path = arg
    if path == None : raise Exception, 'Please, specify a path'
    opts['path'] = path
    return opts

def output (k, v, fmt='%s') :
    sys.stderr.write (('%s\t' + fmt + '\n') % (k, v))

def main () :
    try :
        dc = Cndc (parse ())
        dc.deadlock (sys.stdout)
        r = dc.result
        r['stat'] = 'ok'
    except Exception, e :
        r = {}
        r['stat'] = 'error: %s' % str (e)

    l = list (r)
    l.sort ()
    for k in l : output (k, r[k])

if __name__ == '__main__' :
    main ()
    sys.exit (0)

# vi:ts=4:sw=4:et:
