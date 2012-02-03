
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
        if not 'conflicts' in opt : opt['conflicts'] = 'binary'
        self.opt['conflicts'] = opt['conflicts']
        if not 'symmetric' in opt : opt['symmetric'] = 'linear'
        self.opt['symmetric'] = opt['symmetric']
        if not 'disabled' in opt : opt['disabled'] = 'all'
        self.opt['disabled'] = opt['disabled']
#        if not 'reductions' in opt : opt['reductions'] = 'y'
#        self.opt['reductions'] = opt['reductions'] != 'n'

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
        self.result['opts'] = 'sym=%s asym=%s dis=%s' % \
                (self.opt['symmetric'], self.opt['conflicts'], self.opt['disabled'])

    def deadlock (self) :
#        self.__print_asym (sys.stdout)
#        self.u.stats (sys.stdout)
#        sys.exit (0)

        if self.opt['cnf'] :
            s = self.__dl_cnf ()
#            self.__dl_cnf_debugmodel ()
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

    # FIXME unused, remove !!
    def __dl_net_reduce_plain (self) :
        found = True
        while found :
            found = False;
            evs = set (self.u.events[1:])
            for e in evs :
                if e.iscutoff : continue
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
            for x in cls : out += str (x) + ' '
            out += '0\n'

#        l1 = []
#        for cls in self.cnf_clauses :
#            l = list (cls)
#            l.sort ()
#            l1.append (' '.join (str (x) for x in l) + ' 0\n')
#        l1.sort ()
#        for x in l1 : out += x

        # decision variables
#        out += '\n'
#        out += '90000001 2 1 0\n\n' # no variable, default true
#        out += '90000001 1 1 0\n\n' # all variables, default true
#        out += '90000001 1 2 0\n\n' # all variables, default false

        # events are decision variables
#        for e in self.u.events[1:] :
#            if e.iscutoff : continue
#            atm = self.__dl_cnf_prop (e)
#            default = 1 # true
#            default = 2 # false
#            out += '90000000 %d %d 0\n' % (atm, default)

        # dump the variable mapping
        out += '\n'
        l = self.cnf_prop_tab.keys()
        l.sort ()
        for k in l :
            out += 'c %s => %s\n' % (k, self.cnf_prop_tab[k])
        return out

    def __dl_cnf_config (self) :
        # a configuration is closed
        for e in self.u.events[1:] :
            if e.iscutoff : continue
            s = set ([c.pre for c in e.pre | e.cont if c.pre])
            if not s : continue
            atm = -self.__dl_cnf_prop (e)
#            atm_h = self.__dl_cnf_prop ((e, 'h_%s' % e)) # blocker
#            self.cnf_clauses.add (frozenset ([atm, atm_h])) # blocker
            for ep in s :
                self.cnf_clauses.add (frozenset ([atm, self.__dl_cnf_prop (ep)]))
#                self.cnf_clauses.add (frozenset ([atm, -atm_h, self.__dl_cnf_prop (ep)])) # blocker

        # without symmetric conflicts
        assert self.opt['symmetric'] in ['n2', 'linear']
        if self.opt['symmetric'] == 'n2' :
            self.__dl_cnf_symm_all_n2 ()
        else :
            self.__dl_cnf_symm_all_linear ()

        # and without loops in the asymmetric conflict
        self.__dl_cnf_asym ()

    def __dl_cnf_asym (self) :

        # generate asymmetric conflict graph (without sym. conflicts)
        g = self.u.asym_graph (False)

        # search for sccs
        sccs = networkx.algorithms.strongly_connected_components (g)
        sccs = [x for x in sccs if len (x) >= 2]
        sizes = []
        assert self.opt['conflicts'] in ['trans', 'binary', 'unary']
        for x in sccs :
            h = self.__reduce (g.subgraph (x))
#            h = g.subgraph (x)
#            db (len (x), len (h))
            sizes.append ((len (x), len (h)))
            if self.opt['conflicts'] == 'trans' :
                self.__dl_cnf_trans_scc (h)
            elif self.opt['conflicts'] == 'binary' or self.opt['conflicts'] == 'unary' :
                self.__dl_cnf_po_scc (h)
            else :
                raise Exception, 'conflicts=%s: not implemented' % self.opt['conflicts']
        self.result['sccs'] = repr (sizes)

    def __dl_cnf_symm_all_n2 (self) :
        for c in self.u.conds[1:] :
            l = [e for e in c.post if not e.iscutoff]
            if len (l) < 2 : continue
            for i in xrange (len (l)) :
                atm = - self.__dl_cnf_prop (l[i])
                for j in xrange (i + 1, len (l)) :
                    atm1 = - self.__dl_cnf_prop (l[j])
                    self.cnf_clauses.add (frozenset ([atm, atm1]))

    def __dl_cnf_symm_all_linear (self) :
        l2 = []
        for c in self.u.conds[1:] :
            l = [e for e in c.post if not e.iscutoff]
            if len (l) < 2 : continue
            for i in xrange (len (l)) : l2.append (self.__dl_cnf_prop (l[i]))
            self.__dl_cnf_symm_linear (l2)
            del l2[:]

    def __dl_cnf_symm_linear (self, l) :
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
        self.__dl_cnf_symm_linear (l2)

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
#            k = 1
            db ('k', k);
            for e in g :
                for ep in g[e] :
                    self.__dl_cnf_bin_ord (e, ep, k)
        else :
            k = len (g) - 1
            assert (k >= 2)
            for e in g :
                self.__dl_cnf_una_unary (e, k)
                for ep in g[e] :
                    assert (ep != e)
                    self.__dl_cnf_una_ord (e, ep, k)

    def __dl_cnf_bin_ord (self, a, b, k) :
        # a > b
#        db (repr (a), repr (b), k)
        ai = self.__dl_cnf_prop ((a, 0))
        bi = self.__dl_cnf_prop ((b, 0))
        ci = self.__dl_cnf_prop ((a, b, 0))

        self.cnf_clauses.add (frozenset ([ai, -ci]))
        self.cnf_clauses.add (frozenset ([-ai, -bi, -ci]))
#        self.cnf_clauses.add (frozenset ([-ai, bi, ci])) # optim.

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
#            self.cnf_clauses.add (frozenset ([-ai, bi, ci])) # optim.

        ai = self.__dl_cnf_prop (a)
        bi = self.__dl_cnf_prop (b)
        ci = self.__dl_cnf_prop ((a, b, k - 1))
        self.cnf_clauses.add (frozenset ([-ai, -bi, ci]))

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
        ai = self.__dl_cnf_prop (a)
        bi = self.__dl_cnf_prop (b)
        c = self.__dl_cnf_prop ((a, b))
        self.cnf_clauses.add (frozenset ([-ai, -bi, c]))

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

    def __dl_cnf_conds (self, l) :
        cls = set ()
        for c in l :
            cls.clear ()
            if c.pre and c.pre.iscutoff: continue
#            if len (c.post) + len (c.cont) == 0 : continue; # not sound
            cls.add (self.__dl_cnf_prop (c.label))
            if c.pre : cls.add (- self.__dl_cnf_prop (c.pre))
            for e in c.post :
                if not e.iscutoff : cls.add (self.__dl_cnf_prop (e))
            self.cnf_clauses.add (frozenset (cls))
        
    def __dl_cnf_dead (self) :
        assert self.opt['disabled'] in ['sub', 'all']
        if self.opt['disabled'] == 'all' :
            self.__dl_cnf_dead_all ()
        elif self.opt['disabled'] == 'sub' :
            self.__dl_cnf_dead_sub ()

    def __dl_cnf_dead_all (self) :
#        db ('dead all')
        trans = set ()
        for e in self.u.events[1:] :
#            if e.label in trans : continue
            trans.add (e.label)
            cls = frozenset (- self.__dl_cnf_prop (c.label) for c in e.pre | e.cont)
            self.cnf_clauses.add (cls)
        self.__dl_cnf_conds (self.u.conds[1:])

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

    def __dl_cnf_dead_sub (self) :
#        db ('dead sub')
        trans = set ()
        for e in self.u.events[1:] :
            # if e.pre \cup e.cont minimal, update the marking
            if not self.__dl_cnf_dead_isminimal (e) : continue
            self.__dl_cnf_conds (e.pre | e.cont)
            
            # and if the f(e) not yet considered, state it is disabled
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
        print m
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

    def __print_asym (self, f) :
        g = networkx.DiGraph ()
        for e in self.u.events[1:] :
            if e.iscutoff : continue
            for c in e.pre :
                if c.pre :
                    g.add_edge (c.pre, e, color=1)
                for ep in c.cont :
                    self.__red_edge (g, ep, e, 2)
            for c in e.cont :
                if c.pre and not c.pre.iscutoff:
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

#    def __red_2 (self, g, v) :
#        # assume that outdeg (v) == 1
#        # check that pre(v) and post(v) is green
#
#        u = g.successors (v)[0]
#        if g[v][u]['color'] != 1 : return False
#        for w in g.predecessors (v) :
#            if g[w][v]['color'] != 1 : return False
#        for w in g.predecessors (v) : g.add_edge (w, u, color=1)
#        g.remove_node (v)
#        return True

    def __red_3 (self, g, v) :
        return False

#    def __red_4 (self, g, v) :
#        # assume that outdeg (v) == 1
#        # check that pre(v) is red and post(v) is green
#
#        u = g.successors (v)[0]
#        if g[v][u]['color'] != 1 : return False
#        for w in g.predecessors (v) :
#            if g[w][v]['color'] != 2 : return False
#        for w in g.predecessors (v) : g.add_edge (w, u, color=2)
#        g.remove_node (v)
#        return True

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

# vi:ts=4:sw=4:et:
