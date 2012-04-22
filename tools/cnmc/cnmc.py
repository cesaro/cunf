
import os
import sys
import time
import math
import networkx
import subprocess
import ptnet

def db (*msg) :
    s = ' '.join (str(x) for x in msg)
    sys.stdout.write ('cnmc: ' + s + '\n')

class Cnmc (object) :
    def __init__ (self, opt) :
        self.opt = {}

        # general options
        self.opt['path'] = opt['path']
        self.opt['verbose'] = 'verbose' in opt

        # deadlock options
        self.opt['assert'] = 'assert' in opt

        # other fields
        self.result = {}
        self.u = ptnet.unfolding.Unfolding (self.opt['assert'])

        self.cnf_prop_nr = 0
        self.cnf_prop_tab = {}
        self.cnf_clauses = set ()

        self.stubborn = {}
        self.sfp = {}
        self.asymg = None
        self.sccs = []

        self.t_s = set ()
        self.c_s = set ()
        self.post_t = {}
        self.prec_t = {}
        self.post_p = {}
        self.inv_f = {}

        self.ndecv = 0

        self.cnf_prop_nr = 0
        self.cnf_prop_tab = {}
        self.cnf_clauses = set ()

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

        self.result['opts'] = '2tree/all bin/all t-based/all'

    def deadlock (self) :
#        self.__dl_cnf_debugmodel ()
#        i = 0;
#        for e in self.u.events[1:] :
#            if self.__is_stubborn (e) :
#                print 'stubborn', e
#                i += 1
#        print '%st:', 100.0 * i / len (self.u.events[1:])

        s = self.__dl_cnf ()
        sys.stderr.write (s)

    def __dl_cnf (self) :
        # nothing to do if there is zero cutoffs
        if self.u.nr_black == 0 :
            return 'c zero cutoffs!\np cnf 1 1 1\n1 0\n'

        # models are configurations in which all events are disabled
#        self.__compute_30 ()
        self.__compute_sccs ()
#        self.__compute_stubborn ()

        self.__causal ()
        self.result['var_ev'] = self.cnf_prop_nr
        self.ndecv = self.cnf_prop_nr
#        db ('after causal', self.cnf_prop_nr)
        self.__symm_all_ktree (4)
#        db ('after symm ktree', self.cnf_prop_nr)
#        self.__symm_sub_ktree (4)
        self.__asym ()
#        db ('after asym', self.cnf_prop_nr)

#        self.__fix ()
#        self.__dl_cnf_dead_all_ev_1 ()
#        self.__dl_cnf_dead_all_ev_2 ()
#        self.__dis_tbased_all ()
        self.__reach ()
#        db ('after dis', self.cnf_prop_nr)
#        self.__dis_tbased_sub ()

#        i = 0
#        for e in self.u.events[1:] :
#            #if not e.isblack : continue
#            # lundi 27 fevrier 2012, 18:51:41 (UTC+0100) deberia ser asi:
#            if not e.isblack or not spf.__is_stubborn (e): continue
#            s = self.__sfp (e)
#            if len (s) == 1 :
#                ep = list (s)[0]
#                self.cnf_clauses.add (frozenset ([-self.__dl_cnf_prop (ep)]))
#                print 'cutoff', repr (e), 'implies not', repr (ep)
#                i += 1
#        print '%note:', 100.0 * i / len (self.u.events[1:])
#
#        l2m = 0
#        l2 = 0
#        l2p = 0
#        for x in self.cnf_clauses :
#            if len (x) == 1 : l2m += 1
#            if len (x) == 2 : l2 += 1
#            if len (x) > 2 : l2p += 1
#        print '%size=1:', 100.0 * l2m / len (self.cnf_clauses)
#        print '%size=2:', 100.0 * l2 / len (self.cnf_clauses)
#        print '%size>2:', 100.0 * l2p / len (self.cnf_clauses)
            
        self.ndecv = 0
        if self.ndecv == 0 : self.ndecv = self.cnf_prop_nr
        out = 'p cnf %d %d %d\n' % (self.cnf_prop_nr, len (self.cnf_clauses), self.ndecv)
        for cls in self.cnf_clauses :
            for x in cls : out += str (x) + ' '
            out += '0\n'

        # dump the variable mapping
        out += '\n'
        l = self.cnf_prop_tab.keys()
        l.sort ()
        for k in l :
            out += 'c %s => %s\n' % (k, self.cnf_prop_tab[k])
        return out

    def __compute_30 (self) :
        # compute inv_f for places and post'_p (= postset + context)
        for c in self.u.conds[1:] :
            if not c.label in self.inv_f :
                self.inv_f[c.label] = set ()
                self.post_p[c.label] = set ()
            self.inv_f[c.label].add (c)
            self.post_p[c.label] |= set (e.label for e in c.post | c.cont)

        # compute post_t and prec'_t ( = preset + context of t)
        for e in self.u.events[1:] :
            if e.label in self.post_t : continue
            self.post_t[e.label] = set (c.label for c in e.post)
            self.prec_t[e.label] = set (c.label for c in e.pre | e.cont)

        # compute T_s
        for e in self.u.events[1:] :
            if e.isgray :
                for p in self.post_t[e.label] :
                    self.t_s |= self.post_p[p]

        # compute C_s := inv_f (prec_t (T_s))
        for t in self.t_s :
            for p in self.prec_t[t] :
                self.c_s |= self.inv_f[p]

#        # compute C_s (alternative definition in the notebook)
#        for e in self.u.events[1:] :
#            if e.isgray :
#                for p in self.post_t[e.label] :
#                    for t in self.post_p[p] :
#                        u = self.post_t[e.label] & self.pre_t[t]
#                        v = self.pre_t[t] - self.post_t[e.label]
#                        
        # ()

    def __fix (self) :
        # computes (32)
        # all T_s are disabled
#        db ('fix starts')
        for t in self.t_s :
#            db ('fix disabled', self.prec_t[t])
            l = [-self.__dl_cnf_prop (p) for p in self.prec_t[t]]
            self.cnf_clauses.add (frozenset (l))

        # marking of f(C_s)
        for c in self.c_s :
            if c.pre :
                if c.pre.isblack : continue
                if not self.__is_stubborn (c.pre) :
                    s = set ([c.pre])
                else :
                    s = self.__sfp (c.pre)
                s = set (-self.__dl_cnf_prop (e) for e in s)
            else :
                s = set ()
            s |= set (self.__dl_cnf_prop (e) for e in c.post if not e.isblack)
            s.add (self.__dl_cnf_prop (c.label))
            self.cnf_clauses.add (frozenset (s))
#            db ('fix marking', s)
#        db ('fix ends')

    def __causal (self) :
        # a configuration is closed
        for e in self.u.events[1:] :
            if e.isblack or self.__is_stubborn (e) : continue
            s = self.__sfp (e)
            atm = -self.__dl_cnf_prop (e)
            if not s : continue
            for ep in s :
                self.cnf_clauses.add (frozenset ([atm, self.__dl_cnf_prop (ep)]))

    def __compute_sccs (self) :
        # generate asymmetric conflict graph (without sym. conflicts)
        self.asymg = self.u.asym_graph (False)

        # search for sccs
        sccs = networkx.algorithms.strongly_connected_components (self.asymg)
        self.sccs = [x for x in sccs if len (x) >= 2]

    def __asym (self) :
        sizes = []
        for x in self.sccs :
            h = self.__reduce (self.asymg.subgraph (x))
#            h = self.asymg.subgraph (x)
#            db (len (x), len (h))
            sizes.append ((len (x), self.asymg.subgraph(x).size (), len (h),
                        h.size ()))
            self.__dl_cnf_po_scc (h)
        self.result['sccs'] = repr (sizes)

    def __dl_cnf_symm_ismaximal (self, c) :
        # c tiene 2 en el postset al menos, y no cutoffs
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

    def __symm_sub_ktree (self, k) :
        l2 = []
        for c in self.u.conds[1:] :
            l = [e for e in c.post if not e.isblack]
            if len (l) < 2 : continue
            if not self.__dl_cnf_symm_ismaximal (c) : continue
            self.amo_ktree ([self.__dl_cnf_prop (e) for e in l], k)
#            self.__dl_cnf_symm_2tree ([self.__dl_cnf_prop (e) for e in l])
#            db ('2tree', [self.__dl_cnf_prop (e) for e in l])

    def __symm_all_ktree (self, k) :
        l2 = []
        for c in self.u.conds[1:] :
            l = [e for e in c.post if not e.isblack]
            if len (l) < 2 : continue
            self.amo_ktree ([self.__dl_cnf_prop (e) for e in l], k)
#            self.__dl_cnf_symm_2tree ([self.__dl_cnf_prop (e) for e in l])
#            db ('2tree', [self.__dl_cnf_prop (e) for e in l])

    def __dl_cnf_symm_2tree (self, l) :
        # for n >= 3 items, produce n-1 new vars and 3n-5 clauses
        if len (l) <= 1 : return
        l.sort ()
        if len (l) == 2 :
            self.cnf_clauses.add (frozenset ([-l[0], -l[1]]))
            return

        l2 = []
        if len (l) & 1 : l2.append (l.pop ())
        for i in xrange (0, len (l), 2) :
            atm1 = l[i]
            atm2 = l[i + 1]
            atm = self.__dl_cnf_prop ((atm1, atm2))

            self.cnf_clauses.add (frozenset ([-atm1, -atm2]))
            self.cnf_clauses.add (frozenset ([-atm1, atm]))
            self.cnf_clauses.add (frozenset ([-atm2, atm]))
            l2.append (atm)
        self.__dl_cnf_symm_2tree (l2)

    def amo_standard (self, l) :
        # for n >= 0 items, produces (n^2-n)/2 clauses, no new variables
        for i in xrange (len (l)) :
            for j in xrange (i + 1, len (l)) :
                self.cnf_clauses.add (frozenset ([-l[i], -l[j]]))

    def amo_ktree (self, l, k) :
        if len (l) <= 1 : return
        l.sort ()
        if len (l) == 2 :
            self.cnf_clauses.add (frozenset ([-l[0], -l[1]]))
            return
        assert (k >= 2)
        if len (l) <= k :
            self.amo_standard (l)
            return

        l2 = l[: len (l) % k]
        del l[: len (l) % k]
        for i in xrange (0, len (l), k) :
            l3 = l[i:i + k]
            v = self.__dl_cnf_prop (('amo_ktree', frozenset (l3)))
            for vi in l3 : self.cnf_clauses.add (frozenset ([-vi, v]))
            self.amo_standard (l3)
            l2.append (v)
        self.amo_ktree (l2, k)

    def __dl_cnf_po_scc (self, g) :
#        self.__dl_cnf_trans_scc (g)
#        return

        k = int (math.ceil (math.log (len (g), 2)))
#        k = 1
#        db ('k', k);
        for e in g :
            for ep in g[e] :
                self.__dl_cnf_bin_ord (e, ep, k)
#                self.__dl_cnf_una_ord (e, ep, len (g))
#            if len (g[e]) : self.__dl_cnf_una_unary (e, len (g))

    def __dl_cnf_bin_ord (self, a, b, k) :
        # a > b
#        db (repr (a), repr (b), k)
        ai = self.__dl_cnf_prop ((a, 0))
        bi = self.__dl_cnf_prop ((b, 0))
        ci1 = self.__dl_cnf_prop ((a, b, 0))

        self.cnf_clauses.add (frozenset ([ai, -ci1]))
        self.cnf_clauses.add (frozenset ([-ai, -bi, -ci1]))
#        self.cnf_clauses.add (frozenset ([-ai, bi, ci])) # optim.

        for i in xrange (1, k) :
            ai = self.__dl_cnf_prop ((a, i))
            bi = self.__dl_cnf_prop ((b, i))
            ci = self.__dl_cnf_prop ((a, b, i))

            self.cnf_clauses.add (frozenset ([ai, -bi, -ci]))
            self.cnf_clauses.add (frozenset ([-ai, -bi, -ci1, ci]))
            self.cnf_clauses.add (frozenset ([-ai, -bi, ci1, -ci]))
            self.cnf_clauses.add (frozenset ([ai, bi, -ci1, ci]))
            self.cnf_clauses.add (frozenset ([ai, bi, ci1, -ci]))
#            self.cnf_clauses.add (frozenset ([-ai, bi, ci])) # optim.
            ci1 = ci

        if self.__is_stubborn (a) :
            l = [-self.__dl_cnf_prop (e) for e in self.__sfp (a)]
        else :
            l = [-self.__dl_cnf_prop (a)]

        if self.__is_stubborn (b) :
            l += [-self.__dl_cnf_prop (e) for e in self.__sfp (b)]
        else :
            l += [-self.__dl_cnf_prop (b)]
        l.append (self.__dl_cnf_prop ((a, b, k - 1)))
        self.cnf_clauses.add (frozenset (l))

    def __tbased_mark (self, c) :
        if c.pre and c.pre.isblack : return
        cls = set ([self.__dl_cnf_prop (c.label)])
        if c.pre :
            if self.__is_stubborn (c.pre) :
                for e in self.__sfp (c.pre) :
                    cls.add (- self.__dl_cnf_prop (e))
            else :
                cls.add (- self.__dl_cnf_prop (c.pre))
        l = [e for e in c.post if not e.isblack]
        if len (l) == 1 and self.__is_stubborn (l[0]) :
            for e in self.__sfp (l[0]) :
                s = frozenset (cls | set ([self.__dl_cnf_prop (e)]))
                self.cnf_clauses.add (s)
            return
        for e in l : cls.add (self.__dl_cnf_prop (e))
        self.cnf_clauses.add (frozenset (cls))

    def __cut (self, c) :
        if c.pre and c.pre.isblack : return
        atm = self.__dl_cnf_prop (c)
        if c.pre :
            if self.__is_stubborn (c.pre) :
                for e in self.__sfp (c.pre) :
                    l = [-atm, self.__dl_cnf_prop (e)]
#                    print 'cut pred stubb', l
                    self.cnf_clauses.add (frozenset (l))
            else :
                l = [-atm, self.__dl_cnf_prop (c.pre)]
#                print 'cut pred nonstubb', l
                self.cnf_clauses.add (frozenset (l))
        l = [e for e in c.post if not e.isblack]
        if len (l) == 1 and self.__is_stubborn (l[0]) :
            l2 = [-self.__dl_cnf_prop (e) for e in self.__sfp (l[0])]
            l2.append (-atm)
#            print 'cut post 1stubb', l2
            self.cnf_clauses.add (frozenset (l2))
            return
        for e in l :
            atm1 = self.__dl_cnf_prop (e)
#            print 'cut post nonstubb', [-atm, -atm1]
            self.cnf_clauses.add (frozenset ([-atm, -atm1]))

    def __reach (self) :
#        cover = ['17-on/2x2']
#        cover = ['17-on/2x2', '16-off/2x2']
#        cover = ['17-on/2x2', '10-off/1x2', '14-off/2x1', '8-off/1x1']
#        cover = ['1-on/0x0', '0-off/0x0']

         # n = 3
        cover = ['11-on/1x1', '23-on/2x3', '26-off/3x1', '21-on/2x2']

         # n = 2
#        cover = ['9-on/1x1', '7-on/1x0']
#        cover = ['9-on/1x1', '6-off/1x0']
#        cover = ['9-on/1x1', '15-on/2x1']

        # n = 4
#        cover = ['49-on/4x4', '38-off/3x4', '46-off/4x3']

        d = {}
        for c in self.u.conds[1:] :
            if not c.label in cover : continue
            if c.pre and c.pre.isblack : continue
            if not c.label in d : d[c.label] = []
            d[c.label].append (c)
            self.__cut (c)
        for p in cover :
            if not p in d : d[p] = []
            l = [self.__dl_cnf_prop (c) for c in d[p]]
            self.cnf_clauses.add (frozenset (l))

    def __dis_tbased_all (self) :
#       first encoding: ! (pre (t)) for all t; p <- e & ~e1 & ... & ~en
#        db ('dead all')
        tseen = set ()
        pneed = set ()
        for e in self.u.events[1:] :
            if not e.label in tseen :
                tseen.add (e.label)
                cls = [- self.__dl_cnf_prop (c.label) for c in e.pre | e.cont]
                self.cnf_clauses.add (frozenset (cls))
                pneed |= set (c.label for c in e.pre | e.cont)
        
        for c in self.u.conds[1:] :
            if c.label in pneed :
                self.__tbased_mark (c)

    def __dis_tbased_sub (self) :
#       first encoding on minimal sets of pre(t) + cont(t) for t in T

        tmp = set ()
        minimal = []
        tseen = set ()
        for e in self.u.events[1:] :
            if e.label in tseen : continue
            tseen.add (e.label)
            new = frozenset (c.label for c in e.pre | e.cont)
            tmp.clear ()
            for elem in minimal :
#                if new >= elem : db ('discarding', new);
                if new >= elem : new = None; break
                if new < elem : tmp.add (elem)
            for elem in tmp : minimal.remove (elem)
#            for elem in tmp : db ('removing', elem)
            if new : minimal.append (new)
#            if new : db ('taking', new)
        tmp.clear ()

        pneed = set ()
        for elem in minimal :
            cls = frozenset (- self.__dl_cnf_prop (p) for p in elem)
            self.cnf_clauses.add (cls)
            pneed |= elem

        for c in self.u.conds[1:] :
            if c.label in pneed :
                self.__tbased_mark (c)

    def __dl_cnf_prop (self, x) :
        if x in self.cnf_prop_tab : return self.cnf_prop_tab[x]
        self.cnf_prop_nr += 1
        self.cnf_prop_tab[x] = self.cnf_prop_nr
        return self.cnf_prop_nr

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

    def __compute_stubborn (self) :
        # exclude events consuming or reading conditions in C_s
        for c in self.c_s :
            for e in c.post | c.cont :
                self.stubborn[e] = False

        # for the remaining events, compute recursively
        for e in self.u.events[1:] :
            self.__is_stubborn (e)

    def __is_stubborn (self, e) :
        return False
#       st3: post (pre (e) U cont (e)) without blacks subset of {e}
        if e in self.stubborn : return self.stubborn[e]

        # post (pre + cont) = e
        b = True
        for c in e.pre | e.cont :
            for ep in c.post :
                if not ep.isblack and ep != e :
                    b = False
                    break

# only needed for the condition encoding?
#        # cont (pre) are all stubborn
#        for c in e.pre :
#            for ep in c.cont :
#                if b and not self.__is_stubborn (ep) :
#                    b = False
#                    break

        self.stubborn[e] = b
        return b

    def __sfp (self, e) :
        if e in self.sfp : return self.sfp[e]
        s = set ();
        for c in e.pre | e.cont :
            if c.pre == None : continue
            if self.__is_stubborn (c.pre) :
                s |= self.__sfp (c.pre)
            else :
                s.add (c.pre)
        self.sfp[e] = s
        return self.sfp[e]






# *************** UNUSED CODE **********************

    def __print_asym (self, f) :
        g = networkx.DiGraph ()
        for e in self.u.events[1:] :
            if e.isblack : continue
            for c in e.pre :
                if c.pre :
                    g.add_edge (c.pre, e, color=1)
                for ep in c.cont :
                    self.__red_edge (g, ep, e, 2)
            for c in e.cont :
                if c.pre and not c.pre.isblack:
                    g.add_edge (c.pre, e, color=1)
        
        sccs = networkx.algorithms.strongly_connected_components (g)
        sccs = [x for x in sccs if len (x) >= 2]

        f.write ('digraph {\n')
        sizes = []
        for x in sccs :
            h = g.subgraph (x)
            u = self.__reduce (self.__copy_g (h))
            sizes.append ((len (h), len (u)))
            for e in h :
                if e in u : f.write (' "%s" [style=filled fillcolor=gray90];\n' % repr (e))
                for ep in h[e] :
                    c = 'green' if h[e][ep]['color'] == 1 else 'red'
                    f.write (' "%s" -> "%s" [color=%s];\n' % (repr(e), repr(ep), c))

            for e in u :
                for ep in u[e] :
                    c = 'green' if u[e][ep]['color'] == 1 else 'red'
                    f.write (' "x%s" -> "x%s" [color=%s];\n' % (repr(e), repr(ep), c))

        f.write ('\n graph [label="%d sccs of sizes %s"];\n}\n' % (len (sccs), sizes))
        sys.stderr.write ('%s %d %s\n' % (self.opt['path'], len (sccs), sizes))

    def __copy_g (self, g) :
        h = networkx.DiGraph ()
        for (u, v) in g.edges () : h.add_edge (u, v, color=g[u][v]['color'])
        return h

    # FIXME unused, remove !!
    def __dl_net_reduce_plain (self) :
        found = True
        while found :
            found = False;
            evs = set (self.u.events[1:])
            for e in evs :
                if e.isblack : continue
#                if len (e.pre) != 1 or len (e.post) != 1 : continue # hack
                s = set ()
                for c in e.pre : s |= c.post
                if len (s) == 1 :
                    self.__dl_event_reduce_prop (e)
                    #found = True # removed for reduce_prop

    # FIXME unused, remove !!
    def __dl_net_reduce_plain_db (self) :
        s = set ()
        s.add (self.u.events[1306])
        for x in s : db ('to reduce', x)
        for x in s : self.__dl_event_reduce_prop (x)

    # FIXME unused, remove !!
    def __dl_event_reduce_prop (self, e) :
        s = set (-self.__dl_cnf_prop (c.pre) for c in e.pre if c.pre)
        s.add (self.__dl_cnf_prop (e))
        db ('stubborn propagation clause', s)
        self.cnf_clauses.add (frozenset (s))
        
    # FIXME unused, remove !!
    def __dl_event_reduce (self, e) :
        db ('reduce', e)
        for c in e.pre :
            for cp in e.post :
                pre = c.pre.nr if c.pre != None else 0
                post = [ep.nr for ep in cp.post]
                l = '(%s)-(%s)' % (repr (c), repr (cp))
                cpp = self.u.add_cond (l, pre, post, [])
                db ('pre.pre', c.pre)
                db ('post.post', cp.post.pop ())
                db ('pre', pre)
                db ('post', post)
                db ('label', l)
                db ('new cond', cpp)

        s = e.pre | e.post
        db ('removing event', e)
        self.u.rem_event (e.nr)
        db ('removing old conditions', s)
        for c in s : self.u.rem_cond (c.nr)

        db ('new cond again', cpp)
        db ('event in pre of newcond', cpp.pre)
        for x in cpp.post : db ('event in post of newcond', x)

    def __dl_cnf_debugmodel (self) :
        f = open ('/tmp/m')
        t = f.read ()
        f.close ()
        m = []
        var2obj = {}
        for k in self.cnf_prop_tab :
            if self.cnf_prop_tab[k] in var2obj : raise Exception, 'Here!'
            var2obj[self.cnf_prop_tab[k]] = k
            db ('var', self.cnf_prop_tab[k], 'is', k)
        for v in t.split () :
            n = int (v)
            if n == 0 : break
            if n < 0 : continue
            obj = var2obj[n]
            if type (obj) == type (self.u.events[1]) :
                print 'appending', obj
                m.append (obj)
            assert (len (m) == 0 or not m[-1].isblack)
        self.__dl_bc_assert (set (m))

    def __dl_bc (self) :
        '''Builds a boolean circuit, satisfisable iff there is a deadlock'''
        out = 'BC1.0\n\n'

        # nothing to do if there is zero cutoffs
        if self.u.nr_cutoffs == 0 :
            out += 'no_cutoffs := T;\nASSIGN no_cutoffs;\n'
            return out

        # models are configurations
        out += self.__dl_bc_config ()

        # in which all events are disabled (including cutoffs)
        out += self.__dl_bc_dead ()
        return out

    def __dl_cnf_symm_all_n2 (self) :
        for c in self.u.conds[1:] :
            l = [e for e in c.post if not e.isblack]
            if len (l) < 2 : continue
            for i in xrange (len (l)) :
                atm = - self.__dl_cnf_prop (l[i])
                for j in xrange (i + 1, len (l)) :
                    atm1 = - self.__dl_cnf_prop (l[j])
                    self.cnf_clauses.add (frozenset ([atm, atm1]))

    def __dl_cnf_trans_scc (self, g) :
        for e in g :
            if self.__is_stubborn (e) :
                l = [-self.__dl_cnf_prop (ep) for ep in self.__sfp (e)]
            else :
                l = [-self.__dl_cnf_prop (e)]

            for ep in g[e] :
                if self.__is_stubborn (ep) :
                    l2 = [-self.__dl_cnf_prop (epp) for epp in self.__sfp (ep)]
                else :
                    l2 = [-self.__dl_cnf_prop (ep)]
                cls = frozenset (l + l2 + [self.__dl_cnf_prop ((e, ep))])
                self.cnf_clauses.add (cls)

        for e in g :
            for e1 in g :
                if e == e1 : continue
                atm = - self.__dl_cnf_prop ((e, e1))
                atm1 = - self.__dl_cnf_prop ((e1, e))
                self.cnf_clauses.add (frozenset ([atm, atm1]))

        for e in g :
            for e1 in g :
                if e == e1 : continue
                atm = - self.__dl_cnf_prop ((e, e1))
                for e2 in g[e1] :
                    if e == e2 or e2 in g[e] : continue
                    atm1 = - self.__dl_cnf_prop ((e1, e2))
                    atm2 = self.__dl_cnf_prop ((e, e2))
                    self.cnf_clauses.add (frozenset ([atm, atm1, atm2]))

    def __dl_cnf_una_unary (self, a, k) :
        aim1 = self.__dl_cnf_prop ((a, 0))
#        db (aim1, 'is', (a, 0))
        for i in xrange (1, k) :
            ai = self.__dl_cnf_prop ((a, i))
            self.cnf_clauses.add (frozenset ([-ai, aim1]))
#            db (ai, 'is', (a, i))
#            db ('clause', [-ai, aim1])
            aim1 = ai

    def __dl_cnf_una_ord (self, a, b, k) :
        # a > b
        if self.__is_stubborn (a) :
            l = [-self.__dl_cnf_prop (e) for e in self.__sfp (a)]
        else :
            l = [-self.__dl_cnf_prop (a)]

        if self.__is_stubborn (b) :
            l += [-self.__dl_cnf_prop (e) for e in self.__sfp (b)]
        else :
            l += [-self.__dl_cnf_prop (b)]
        c = self.__dl_cnf_prop ((a, b))
        l.append (c)
        self.cnf_clauses.add (frozenset (l))

        ai = self.__dl_cnf_prop ((a, 0))
        bi = self.__dl_cnf_prop ((b, k - 1))
        self.cnf_clauses.add (frozenset ([-c, ai]))
        self.cnf_clauses.add (frozenset ([-c, -bi]))
#        db (ai, 'is', (a, 0))
#        db (bi, 'is', (b, k-1))
#        db ('clause', [-c, ai])
#        db ('clause', [-c, -bi])
        for i in xrange (k - 1) :
            bi = self.__dl_cnf_prop ((b, i))
            ai = self.__dl_cnf_prop ((a, i + 1))
            self.cnf_clauses.add (frozenset ([-c, -bi, ai]))
#            db (bi, 'is', (b, i))
#            db (ai, 'is', (a, i+1))
#            db ('clause', [-c, -bi, ai])

    def __dl_cnf_dead_isminimal (self, e) :
        m = self.u.mark
        self.u.mark += 1
        e.count = len (e.pre) + len (e.cont)
#        db ('isminimal', e)
#        db ('count', e.count)
        for c in e.pre | e.cont :
            for ep in c.post | c.cont :
                if e == ep : continue
                if ep.mark != m :
                    ep.mark = m
                    ep.count = 1
                else :
                    ep.count += 1
                if ep.count == len (ep.pre) + len (ep.cont) :
                    if ep.count == e.count and e.nr < ep.nr : continue
#                    db ('not minimal', e, 'due to', ep)
                    return False
#        for c in e.pre | e.cont :
#            for ep in c.post | c.cont : db ('ep', repr (ep), 'target', len (ep.pre) + len (ep.cont), 'count', ep.count)
        return True

    def __dl_cnf_dead_all_ev_1 (self) :
#       computes (variation of 17, with context)
        for e in self.u.events[1:] :
            if self.__is_stubborn (e) and not e.isblack : continue
            s = set (-self.__dl_cnf_prop (ep) for ep in self.__sfp (e))
            for c in e.pre | e.cont :
                for ep in c.post :
                    if not ep.isblack : s.add (self.__dl_cnf_prop (ep))
            self.cnf_clauses.add (frozenset (s))

    def __dl_cnf_dead_all_ev_2 (self) :
#       computes (variation of 17, with context, and conditions)
        seen = set ()
        for e in self.u.events[1:] :
            if self.__is_stubborn (e) and not e.isblack : continue
            s = set (-self.__dl_cnf_prop (ep) for ep in self.__sfp (e))
            for c in e.pre | e.cont :
                if c in seen :
                    s.add (self.__dl_cnf_prop (c))
                    continue
                l = [self.__dl_cnf_prop (ep) for ep in c.post
                        if not ep.isblack]
                if not l : continue;
                seen.add (c)
                s.add (self.__dl_cnf_prop (c))
                l.append (-self.__dl_cnf_prop (c))
                self.cnf_clauses.add (frozenset (l))
            self.cnf_clauses.add (frozenset (s))

# vi:ts=4:sw=4:et:
