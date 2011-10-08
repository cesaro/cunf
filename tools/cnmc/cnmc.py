
import sys
import time
import math
import networkx
import subprocess
import ptnet

def db (*msg) :
    s = ' '.join (str(x) for x in msg)
    sys.stderr.write ('cnmc: ' + s + '\n')

class Cnmc (object) :
    def __init__ (self, opt) :
        self.opt = {}
        
        # general options
        self.opt['path'] = opt['path']
        self.opt['verbose'] = 'verbose' in opt

        # deadlock options
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
        self.result['rd-t'] = t1 - t

    def deadlock (self) :
        self.result = {}

        t = time.clock ()
        s = self.__dl_bc ()
        t1 = time.clock ()
        self.result['u2bc-t'] = t1 - t

        if self.opt['print'] :
            sys.stderr.write (s)
            return

        m = self.__bczchaff (s)
        t2 = time.clock ()
        self.result['sol-t'] = t2 - t1

        if not m :
            self.result['result'] = 'LIVE'
            return

        if self.u.nr_cutoffs == 0 :
            self.result['result'] = 'DEAD-EVERY'
            return

        self.result['result'] = 'DEAD'
        if self.opt['assert'] or self.opt['verbose'] :
            conf = self.__dl_model2config (m)
            if self.opt['verbose'] :
                self.result['config'] = ' '.join (repr (e) for e in conf)
            if self.opt['assert'] :
                self.__dl_bc_assert (conf)

    def __bczchaff (self, s) :
        db ('calling the solver')
        args = ['bczchaff']
        try :
            p = subprocess.Popen (args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                    stderr=None)
        except OSError as e :
            print e
            print 'Probably command `bczchaff\' was not found in the PATH.'
            sys.exit (1)
        db ('done!')
    
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

    def __dl_bc_dead (self) :
        t = time.clock ()
        if self.opt['disabled'] == 'all' :
            out = self.__dl_bc_disabled_all ()
        elif self.opt['disabled'] == 'sub' :
            out = self.__dl_bc_disabled_filter_sub ()
        self.result['dis-t'] = time.clock () - t
        return out

    def __dl_bc_disabled_all (self) :
        db ('encoding deadlock: all events')

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
        db ('encoding deadlock: subset filtering')

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

        db ('generating -%s- encoding for conflicts' % self.opt['conflicts'])
        if self.opt['conflicts'] == 'trans' :
            out = self.__dl_bc_trans ()
        elif self.opt['conflicts'] == 'binary' :
            out = self.__dl_bc_bin ()
        elif self.opt['conflicts'] == 'cycle' :
            out = self.__dl_bc_cycle ()
        else :
            raise Exception, "'%s': unknown conflict encoding" % self.opt['conflicts']

        # and causally closed
        db ('generating causality')
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
        db ('done!')
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
        db ('encoding symmetric conflicts: all sets')
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
        db ('encoding symmetric conflicts: subset filtering')
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
        db (len (sccs), 'non-trivial scc(s) of size(s):')
        db (', '.join (str(len (x)) for x in sccs) + '.')
        i = 0
        for x in sccs :
            db ('scc', x)
            (i, out2) = self.__dl_bc_trans_scc (g.subgraph (x), i)
            out += out2
        if i :
            s = ''
            for j in xrange (i) : s += ',z%d' % j
            out += 'ASSIGN ' + s[1:] + ';\n\n'
        db ('done!')
        self.result['sym-t'] = t1 - t
        self.result['asym-t'] = time.clock () - t1
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

    def __dl_model2config (self, m) :
        conf = set ()
        for k in m :
            if 'e' == k[0] and m[k] : conf.add (self.__dl_prop2ref (k))
        return conf

    def __dl_prop2ref (self, s) :
        assert (len (s))
        assert (s[0] == 'c' or s[0] == 'e')
        idx = int (s[1:])
        if s[0] == 'e' :
            return self.u.events[idx]
        else :
            return self.u.conds[idx]

# vi:ts=4:sw=4:et:
