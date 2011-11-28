
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
        self.opt['cnf'] = 'cnf' in opt
        self.opt['print'] = 'print' in opt
        self.opt['assert'] = 'assert' in opt
        if not 'conflicts' in opt : opt['conflicts'] = 'trans'
        self.opt['conflicts'] = opt['conflicts']
        if not 'symmetric' in opt : opt['symmetric'] = 'all'
        self.opt['symmetric'] = opt['symmetric']
        if not 'disabled' in opt : opt['disabled'] = 'all'
        self.opt['disabled'] = opt['disabled']

        # other fields
        self.result = {}
        self.n = None
        self.u = ptnet.unfolding.Unfolding (self.opt['assert'])

        self.cnf_prop_nr = 0
        self.cnf_prop_tab = {}
        self.cnf_clauses = set ()

        # load the model
        t = time.clock ()
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

        t1 = time.clock ()
        self.result['read'] = t1 - t

    def deadlock (self) :
        if self.opt['cnf'] :
            s = self.__dl_cnf ()
        else :
            s = self.__dl_bc ()
        self.result['gen'] = time.clock ()

        if self.opt['print'] or self.opt['cnf']:
            sys.stderr.write (s)
            return

        m = self.__bczchaff (s)
        self.result['solve'] = os.times () [2]

        if not m :
            self.result['result'] = 'LIVE'
            return

        if self.u.nr_cutoffs == 0 :
            self.result['result'] = 'DEAD-EVERY'
            return

        self.result['result'] = 'DEAD'
        if self.opt['assert'] or self.opt['verbose'] :
            conf = self.__dl_bc_model2config (m)
            if self.opt['verbose'] :
                self.result['config'] = ' '.join (repr (e) for e in conf)
            if self.opt['assert'] :
                self.__dl_bc_assert (conf)

    def __dl_cnf_debugmodel (self) :
        f = open ('/tmp/m')
        t = f.read ()
        f.close ()
        m = []
        var2obj = {}
        for k in self.cnf_prop_tab :
            if self.cnf_prop_tab[k] in var2obj : raise Exception, 'Here!'
            var2obj[self.cnf_prop_tab[k]] = k
        for v in t.split () :
            n = int (v)
            if n == 0 : break
            if n < 0 : continue
            obj = var2obj[n]
            if type (obj) == type (self.u.events[1]) :
                print 'appending', obj
                m.append (obj)
            assert (len (m) == 0 or not m[-1].iscutoff)
        self.__dl_bc_assert (set (m))

    def __bczchaff (self, s) :
        args = ['bczchaff']
        try :
            p = subprocess.Popen (args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                    stderr=None)
        except OSError as e :
            print e
            print 'Probably command `bczchaff\' was not found in the PATH.'
            sys.exit (1)
    
        p.stdin.write (s)
        p.stdin.close ()
    
        r = p.stdout.read ()
        last = False
        m = {}
        for a in r.split () :
            if a == 'Satisfiable' or a == 'Unsatisfiable' : last = True; continue
            assert (not last)
            if a[0] == '~' :
                m[a[1:]] = False
            else :
                m[a] = True
        return m

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

    def __dl_cnf (self) :
        self.cnf_prop_nr = 0
        self.cnf_prop_tab = {}
        self.cnf_clauses = set ()

        # nothing to do if there is zero cutoffs
        if self.u.nr_cutoffs == 0 :
            return 'c zero cutoffs!\np cnf 1 1\n1 0\n'

        # models are configurations in which all events are disabled
        self.__dl_cnf_config ()
        self.__dl_cnf_dead ()

        out = 'p cnf %d %d\n' % (self.cnf_prop_nr, len (self.cnf_clauses))
        for cls in self.cnf_clauses :
            out += ' '.join (str (x) for x in cls) + ' 0\n'
#        for x in self.cnf_prop_tab : print '!', self.cnf_prop_tab[x], repr (x)
        return out

    def __dl_cnf_config (self) :
        # a configuration is closed
        for e in self.u.events[1:] :
            if e.iscutoff : continue
            s = set ([c.pre for c in e.pre | e.cont if c.pre])
            if not s : continue
            atm = -self.__dl_cnf_prop (e)
            for ep in s :
                self.cnf_clauses.add (frozenset ([atm, self.__dl_cnf_prop (ep)]))

        # without symmetric conflicts
        if self.opt['symmetric'] == 'n2' :
            self.__dl_cnf_symm_all_n2 ()
        else :
            self.__dl_cnf_symm_all_logn ()

        # and without loops in the asymmetric conflict
        self.__dl_cnf_asym ()

    def __dl_cnf_asym (self) :

        # generate asymmetric conflict graph (without sym. conflicts)
        g = self.u.asym_graph (False)

        # search for sccs
        sccs = networkx.algorithms.strongly_connected_components (g)
        sccs = [x for x in sccs if len (x) >= 2]
        db (len (sccs), 'non-trivial scc(s) of size(s)', [len (x) for x in sccs])
        i = 0
        if self.opt['conflicts'] == 'trans' :
            for x in sccs : self.__dl_cnf_trans_scc (g.subgraph (x))
        elif self.opt['conflicts'] == 'binary' or self.opt['conflicts'] == 'unary' :
            for x in sccs : self.__dl_cnf_po_scc (g.subgraph (x))
        else :
            raise Exception, 'conflicts=%s: not implemented' % self.opt['conflicts']

    def __dl_cnf_symm_all_n2 (self) :
        for c in self.u.conds[1:] :
            l = [e for e in c.post if not e.iscutoff]
            if len (l) < 2 : continue
            for i in xrange (len (l)) :
                atm = - self.__dl_cnf_prop (l[i])
                for j in xrange (i + 1, len (l)) :
                    atm1 = - self.__dl_cnf_prop (l[j])
                    self.cnf_clauses.add (frozenset ([atm, atm1]))

    def __dl_cnf_symm_all_logn (self) :
        l2 = []
        for c in self.u.conds[1:] :
            l = [e for e in c.post if not e.iscutoff]
            if len (l) < 2 : continue
            for i in xrange (len (l)) : l2.append (self.__dl_cnf_prop (l[i]))
            self.__dl_cnf_symm_logn (l2)
            del l2[:]

    def __dl_cnf_symm_logn (self, l) :
        if len (l) <= 1 : return
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
        self.__dl_cnf_symm_logn (l2)

    def __dl_cnf_trans_scc (self, g) :
        for e in g :
            atm = - self.__dl_cnf_prop (e)
            for ep in g[e] :
                cls = frozenset ([atm, - self.__dl_cnf_prop (ep),
                        self.__dl_cnf_prop ((e, ep))])
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

    def __dl_cnf_po_scc (self, g) :
        if self.opt['conflicts'] == 'binary' :
            k = int (math.ceil (math.log (len (g), 2)))
            db ('k', k);
            for e in g :
                for ep in g[e] :
                    self.__dl_cnf_bin_ord (e, ep, k)
        else :
            for e in g :
                for ep in g[e] :
                    self.__dl_cnf_una_ord (e, ep)

    def __dl_cnf_bin_ord (self, a, b, k) :
#        db (repr (a), repr (b), k)
        ai = self.__dl_cnf_prop ((a, 0))
        bi = self.__dl_cnf_prop ((b, 0))
        ci = self.__dl_cnf_prop ((a, b, 0))

        self.cnf_clauses.add (frozenset ([ai, -ci]))
        self.cnf_clauses.add (frozenset ([-ai, -bi, -ci]))
#        self.cnf_clauses.add (frozenset ([-ai, bi, ci]))

        for i in xrange (1, k) :
            ai = self.__dl_cnf_prop ((a, i))
            bi = self.__dl_cnf_prop ((b, i))
            ci = self.__dl_cnf_prop ((a, b, i))
            ci1 = self.__dl_cnf_prop ((a, b, i - 1))

            self.cnf_clauses.add (frozenset ([ai, -bi, -ci]))
            self.cnf_clauses.add (frozenset ([-ai, -bi, -ci1, ci]))
            self.cnf_clauses.add (frozenset ([-ai, -bi, ci1, -ci]))
            self.cnf_clauses.add (frozenset ([ai, bi, -ci1, ci]))
            self.cnf_clauses.add (frozenset ([ai, bi, ci1, -ci]))
#            self.cnf_clauses.add (frozenset ([-ai, bi, ci]))

        ai = self.__dl_cnf_prop (a)
        bi = self.__dl_cnf_prop (b)
        ci = self.__dl_cnf_prop ((a, b, k - 1))
        self.cnf_clauses.add (frozenset ([-ai, -bi, ci]))

    def __dl_cnf_una_ord (self, a, b) :
        pass

    def __dl_cnf_dead (self) :
        cls = set ()
        for c in self.u.conds[1:] :
            cls.clear ()
            if len (c.post) + len (c.cont) == 0 : continue;
            cls.add (self.__dl_cnf_prop (c.label))
            if c.pre : cls.add (- self.__dl_cnf_prop (c.pre))
            for e in c.post :
                if not e.iscutoff : cls.add (self.__dl_cnf_prop (e))
            self.cnf_clauses.add (frozenset (cls))

        trans = set ()
        for e in self.u.events[1:] :
            if e.label in trans : continue
            trans.add (e.label)
            cls = frozenset (- self.__dl_cnf_prop (c.label) for c in e.pre | e.cont)
            self.cnf_clauses.add (cls)

    def __dl_cnf_prop (self, x) :
        if x in self.cnf_prop_tab : return self.cnf_prop_tab[x]
        self.cnf_prop_nr += 1
        self.cnf_prop_tab[x] = self.cnf_prop_nr
        return self.cnf_prop_nr

    def __dl_bc_dead (self) :
        t = time.clock ()
        if self.opt['disabled'] == 'all' :
            out = self.__dl_bc_disabled_all ()
        elif self.opt['disabled'] == 'sub' :
            out = self.__dl_bc_disabled_filter_sub ()
        self.result['dis'] = time.clock () - t
        return out

    def __dl_bc_disabled_all (self) :
#        db ('deadlock: all')

        out, s, i, t = '', '', 0, set ()
        for e in self.u.events[1:] :
            out += 'd%d:=OR(' % i
            out += ','.join ('~c%d' % c.nr for c in (e.pre | e.cont)) + ');\n'
            t |= e.pre | e.cont
            i += 1
        if i :
            out += 'ASSIGN '
            for j in xrange (i) : s += ',d%d' % j
            out += s[1:] + ';\n\n'
        out += self.__dl_bc_conds (t)
        return out

    def __dl_bc_disabled_filter_sub (self) :
#        db ('deadlock: subset')

        out, i, l, s, tmp = '', 0, [], '', set ()
        for e in self.u.events[1:] :
            a = frozenset (e.pre | e.cont)
            tmp.clear ()
            for b in l :
#                if a >= b : db ('discarding', a);
                if a >= b : a = None; break
                if a < b : tmp.add (b)
            for b in tmp : l.remove (b)
#            for b in tmp : db ('removing', b)
            if a : l.append (a)
#            if a : db ('taking', a)
        tmp.clear ()
        for a in l :
            out += 'd%d:=OR(' % i
            out += ','.join ('~c%d' % c.nr for c in a) + ');\n'
            tmp |= a
            i += 1
        if i :
            out += 'ASSIGN '
            for j in xrange (i) : s += ',d%d' % j
            out += s[1:] + ';\n\n'

        self.result['dis-c'] = i
        self.result['dis-%'] = i * 100.0 / len (self.u.events[1:])
        out += self.__dl_bc_conds (tmp)
        return out

    def __dl_bc_conds (self, l) :
        '''Generates the encoding of conditions'''

        # conditions are marked if pre and not post
        out = ''
        for c in l :
            out += 'c%d:=' % c.nr
            out += 'e%d' % c.pre.nr if c.pre else 'T'
            ll = [e for e in c.post if not e.iscutoff]
            if ll :
                out += '&AND('
                out += ','.join ('~e%d' % e.nr for e in ll)
                out += ')'
            out += ';\n'
        return out

    def __dl_bc_config (self) :
        '''Builds a boolean circuit whose models are configurations'''

#        db ('conflicts:', self.opt['conflicts'])
        if self.opt['conflicts'] == 'trans' :
            out = self.__dl_bc_trans ()
        elif self.opt['conflicts'] == 'binary' :
            out = self.__dl_bc_bin ()
        elif self.opt['conflicts'] == 'cycle' :
            out = self.__dl_bc_cycle ()
        else :
            raise Exception, "'%s': unknown conflict encoding" % self.opt['conflicts']

        # and causally closed
        l = []
        for e in self.u.events[1:] :
            if e.iscutoff : continue
            s = set ([c.pre for c in e.pre | e.cont if c.pre])
            if not s : continue
            out += 'cc%d:=e%d=>AND(' % (e.nr, e.nr)
            out += ','.join ('e%d' % e.nr for e in s)
            out += ');\n'
            l.append (e.nr)
        if l :
            out += 'ASSIGN '
            out += ','.join ('cc%d' % x for x in l)
            out += ';\n\n'
        return out

    def __dl_bc_cycle (self) :
        '''Builds a boolean circuit whose models are sets of events free of
           asymmetric-conflict cycles'''

        # generate asymmetric conflict graph (with sym. conflicts)
        g = self.u.asym_graph (True)

        # search for all elementary circuits in g and filter out those that
        # are contained in another one
        cycles = networkx.algorithms.simple_cycles (g)
#        for x in cycles: db ('cycle', x)
        tmp, rem = set (), set ()
        for x in cycles :
            assert (x)
            y = frozenset (x)
            rem.clear ()
            for z in tmp :
                if z <= y :
#                    db ('discarding', z)
                    y = None;
                    break
                if z > y : rem.add (z)
            for z in rem : tmp.remove (z)
#            for z in rem : db ('removing', z)
            if y : tmp.add (y)
#            if y : db ('taking', y)

        self.result['cy-c'] = len (tmp)
        self.result['cy-%'] = 100 if not cycles else len (tmp) * 100.0 / len (cycles)
        cycles = tmp

        # enumeration of asymmetric cycles
        out, j = '', 0
        for x in cycles :
            out += 'z%d:=~AND(' % j
            out += ', '.join ('e%d' % e.nr for e in x)
            out += ');\n'
            j += 1
        if j :
            s = ''
            for i in xrange (j) : s += ',z%d' % i
            out += 'ASSIGN ' + s[1:] + ';\n'
        return out

    def __dl_bc_symm_all (self) :
#        db ('symmetric conflicts: all')
        i, out = 0, ''
        for c in self.u.conds[1:] :
            l = [e for e in c.post if not e.iscutoff]
            if len (l) < 2 : continue
            out += 'y%d:=[0,1](' % i
            out += ','.join ('e%d' % e.nr for e in l) + ');\n'
            i += 1
        if i :
            s = ''
            for j in xrange (i) : s += ',y%d' % j
            out += 'ASSIGN ' + s[1:] + ';\n\n'
        return out

    def __dl_bc_symm_filter_sub (self) :
#        db ('symmetric conflicts: subset')
        i, out, l, tmp, count, avg = 0, '', [], set (), 0, 0
        for c in self.u.conds[1:] :
            a = frozenset (e for e in c.post if not e.iscutoff)
            if len (a) < 2 : continue
            count += 1
            tmp.clear ()
            for b in l :
#                if a <= b : db ('discarding', a);
                if a <= b : a = None; break
                if b < a : tmp.add (b)
            for b in tmp : l.remove (b)
#            for b in tmp : db ('removing', b)
            if a :
#                db ('taking', a)
                l.append (a)
                avg += len (a)
        for a in l :
            out += 'y%d:=[0,1](' % i
            out += ','.join ('e%d' % e.nr for e in a) + ');\n'
            i += 1
        if i :
            s = ''
            for j in xrange (i) : s += ',y%d' % j
            out += 'ASSIGN ' + s[1:] + ';\n\n'
        self.result['sym-c'] = i
        self.result['sym-%'] = 100 if not count else i * 100.0 / count
        self.result['sym-av'] = 0 if not i else float (avg) / i
        return out

    def __dl_bc_trans (self) :
        # encode symmetric conflicts
        t = time.clock ()
        if self.opt['symmetric'] == 'all' :
            out = self.__dl_bc_symm_all ()
        elif self.opt['symmetric'] == 'sub' :
            out = self.__dl_bc_symm_filter_sub ()
        else :
            raise Exception, "'%s': unknown symmetric encoding" % self.opt['symmetric']
        t1 = time.clock ()

        # generate asymmetric conflict graph (without sym. conflicts)
        g = self.u.asym_graph (False)

        # search for sccs
        sccs = networkx.algorithms.strongly_connected_components (g)
        sccs = [x for x in sccs if len (x) >= 2]
#        db (len (sccs), 'non-trivial scc(s) of size(s):')
#        db (', '.join (str(len (x)) for x in sccs) + '.')
        i = 0
        for x in sccs :
#            db ('scc', x)
            raise Exception, "2011/11/17 The BC encoding doesn't work for contextual nets";
            (i, out2) = self.__dl_bc_trans_scc (g.subgraph (x), i)
            out += out2
        if i :
            s = ''
            for j in xrange (i) : s += ',z%d' % j
            out += 'ASSIGN ' + s[1:] + ';\n\n'
#        db ('done!')
        self.result['sym'] = t1 - t
        self.result['asym'] = time.clock () - t1
        return out

    def __dl_bc_trans_scc (self, g, i) :
#        db ('generating unit path...')
        out = ''
        for (e, ep) in g.edges () :
            assert (x != y)
            out += 'z%d:=e%d&e%d=>p%dx%d;\n' % (i, e.nr, ep.nr, e.nr, ep.nr)
            i += 1

#        db ('generating asymmetry...')
        for e in g :
            for ep in g :
                if e == ep : continue
                out += 'z%d:=p%dx%d=>~p%dx%d;\n' % (i, e.nr, ep.nr, ep.nr,
                        e.nr)
                i += 1

#        db ('generating transitivity...')
        for e in g :
            for ep in g :
                if e == ep : continue
                for epp in g[ep] :
                    if e == epp or ep == epp or epp in g[e] : continue
                    out += 'z%d:=p%dx%d&p%dx%d=>p%dx%d;\n' % (i, e.nr, ep.nr,
                            ep.nr, epp.nr, e.nr, epp.nr);
                    i += 1

#        db ('done!')
        return (i, out)

    def __dl_bc_bin (self) :
        g = self.u.asym_graph (True)
        out, z = '', 0
        k = int (math.ceil (math.log (len (g), 2)))
        for (e, ep) in g.edges () :
            out += 'z%d := e%d & e%d => ord%dx%d;\n' % (z, e.nr, ep.nr, e.nr,
                    ep.nr)
            z += 1
            i = k - 1
            s = 'ord%dx%d := ' % (e.nr, ep.nr)
            while i > 0 :
                out += '(e%d%d & ~e%db%d) | ((e%db%d == e%db%d) & (' % (e.nr,
                                i, ep.nr, i, e.nr, i, ep.nr, i)
                i -= 1
            out += '(e%db0 & ~e%db0)' % (e.nr, ep.nr)
            out += ''.join (')' for a in xrange (k)) + ';\n'

        assert (z)
        out += 'noconflicts := AND ('
        out += ', '.join ('z%d' % j for j in xrange (z))
        out += ');\n'
        return out

    def __dl_bc_assert (self, conf) :
        m = self.u.marking_of (conf)
        assert (0 == len (self.u.enabled (m)))

    def __dl_bc_model2config (self, m) :
        conf = set ()
        for k in m :
            if 'e' == k[0] and m[k] : conf.add (self.__dl_bc_prop2ref (k))
        return conf

    def __dl_bc_prop2ref (self, s) :
        assert (len (s))
        assert (s[0] == 'c' or s[0] == 'e')
        idx = int (s[1:])
        if s[0] == 'e' :
            return self.u.events[idx]
        else :
            return self.u.conds[idx]

# vi:ts=4:sw=4:et:
