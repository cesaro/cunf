
import sys
import net
import networkx

def db (*msg) :
    s = ' '.join (str(x) for x in msg)
    sys.stdout.write ('cnmc: ' + s + '\n')

class Event (object) :
    def __init__ (self, nr, l="", pre=set(), post=set(), cont=set(), iscutoff=False) :
        self.nr = nr
        self.label = l
        self.pre = pre
        self.post = post
        self.cont = cont
        self.iscutoff = iscutoff
        self.mark = 0
        self.count = 0

        for c in pre : c.post.add (self)
        for c in post : c.pre = self
        for c in cont : c.cont.add (self)

    def __repr__ (self) :
        s = '*' if (self.iscutoff) else ''
        return '%se%d:%s' % (s, self.nr, self.label)

    def __str__ (self) :
        s = '*' if (self.iscutoff) else ''
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
        self.nr_cutoffs = 0
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

    def add_event (self, l, pre=[], post=[], cont=[], iscutoff=False) :
        # validate condition identifiers
        if self.sanity_check :
            for i in pre : self.__sane_cond_id (i)
            for i in post : self.__sane_cond_id (i)
            for i in cont : self.__sane_cond_id (i)

        # map to object references
        pr = set(self.conds[i] for i in pre)
        po = set(self.conds[i] for i in post)
        co = set(self.conds[i] for i in cont)
        #db ('new event', l, pre, post, cont, 'cutoff', iscutoff)

        # create the new event and register it
        e = Event (len (self.events), l, pr, po, co, iscutoff)
        self.events.append (e)
        if iscutoff : self.nr_cutoffs += 1
        return e

    def rem_cond (self, nr) :
        if self.sanity_check : self.__sane_cond_id (nr)
        c = self.conds[nr]
        if c.pre == None :
            self.m0.discard (c)
        else :
            c.pre.post.remove (c)
        for e in c.cont : e.cont.remove (c)
        for e in c.post : e.pre.remove (c)
        del c
        if nr != len (self.conds) - 1 :
            self.conds[nr] = self.conds[-1]
            self.conds[nr].nr = nr
            del self.conds[-1]

    def rem_event (self, nr) :
        if self.sanity_check : self.__sane_event_id (nr)
        e = self.events[nr]
        assert (e != None)
        for c in e.pre : c.post.remove (e)
        for c in e.cont : c.cont.remove (e)
        for c in e.post : c.pre = None
        del e
        if nr != len (self.events) - 1 :
            db ('rem_event: ultimo antes del cambio', self.events[-1])
            for x in self.events[-1].pre : db ('rem_event: pre del ultimo antes', x)
            self.events[nr] = self.events[-1]
            self.events[nr].nr = nr
            db ('rem_event: ultimo despues del cambio', self.events[-1])
            for x in self.events[-1].pre : db ('rem_event: pre del ultimo despues', x)
        del self.events[-1]

    def write (self, f, fmt='dot', ctxitems=set(), ctxn=0) :
        if fmt == 'dot' : return self.__write_dot (f)
        if fmt == 'ctxdot' : return self.__write_ctxdot (f, ctxitems, ctxn)
        raise Exception, "'%s': unknown output format" % fmt

    def __write_ctxdot (self, f, items, n) :
        if len (items) != 0 : return self.__write_ctxdot_items (f, items, n)

        f.write ('digraph {\n')
        for e in self.events[1:] :
            self.__write_ctxdot_items (f, set ([e]), n, repr (e), False)
        f.write ('}\n')

    def __write_ctxdot_items (self, f, items, n, prefx='', full=True) :
        self.mark += 1
        m = self.mark

        t = set (items)
        for x in t :
            x.mark = m
            x.count = n
        while len (t) :
            s = t
            t = set ()
            for x in s :
                assert (x.mark == m)
                if x.count <= 0 : continue

                for y in x.post | x.cont :
                    if y.mark == m : continue
                    y.mark = m
                    y.count = x.count - 1
                    t.add (y)
                if type (x.pre) == Event :
                    if x.pre.mark != m :
                        x.pre.mark = m
                        x.pre.count = x.count - 1
                        t.add (x.pre)
                elif type (x.pre) == set :
                    for y in x.pre :
                        if y.mark == m : continue
                        y.mark = m
                        y.count = x.count - 1
                        t.add (y)

        self.__write_dot (f, m, prefx, full)

    def __write_dot (self, f, m=0, prefx='', full=True) :
        if full : f.write ('digraph {\n')
        f.write ('\t/* events */\n')
        f.write ('\tnode\t[shape=box style=filled fillcolor=grey80];\n')
        for e in self.events :
            if e == None or (m != 0 and e.mark != m) : continue
            s = '\t%se%-6d [label="%s:e%d"' % \
                    (prefx, e.nr, e.label, e.nr)
            if (e.iscutoff) : s += ' shape=Msquare'
#            if (e.mark) : s += ' fillcolor=blue'
            f.write (s + '];\n')

        f.write ('\n\t/* conditions, flow and context relations */\n')
        f.write ('\tnode\t[shape=circle fillcolor=gray95];')
        for c in self.conds :
            if c == None or (m != 0 and c.mark != m): continue
            s = '*' if c in self.m0 else ''
            s = '\n\t%sc%-6d [label="%s:c%d%s"];' % (prefx, c.nr, c.label, c.nr, s)
            if (c.pre == None) : s += ' /* initial */\n'
            elif m == 0 or c.pre.mark == m :
                s += '\n\t%se%-6d -> %sc%d;\n' % (prefx, c.pre.nr, prefx, c.nr)

            for e in c.post :
                if m == 0 or e.mark == m :
                    s += '\t%sc%-6d -> %se%d;\n' % (prefx, c.nr, prefx, e.nr)
            for e in c.cont :
                if m == 0 or e.mark == m :
                    s += '\t%sc%-6d -> %se%d [arrowhead=none color=red];\n' \
                        % (prefx, c.nr, prefx, e.nr)
            f.write (s)

        if full : f.write ('}\n')

    def __cuf2unf_readstr (self, f, m) :
        s = ""
        for i in xrange (0, m) :
            t = f.read (1)
            if (len (t) == 0) : break
            if (t == '\x00') : return s
            s += t
        raise Exception, 'Corrupted .cuf file'

    def __cuf2unf_readint (self, f) :
        s = f.read (4)
        if (len (s) != 4) : raise Exception, 'Corrupted .cuf file'
        r = (ord (s[0]) << 24) + (ord (s[1]) << 16)
        r += (ord (s[2]) << 8) + ord (s[3])
        return r

    def read (self, f, fmt='cuf') :
        if fmt != 'cuf' : raise Exception, "'%s': unknown input format" % fmt
        # read first four fields
        nrc = self.__cuf2unf_readint (f)
        nre = self.__cuf2unf_readint (f)
        nrf = self.__cuf2unf_readint (f)
        m = 1 + self.__cuf2unf_readint (f)
        #r, nrc, nre, nrf, m

        # read nre event (nrf cutoff event) labels
        for i in xrange (0, nre - nrf) :
            self.add_event (self.__cuf2unf_readstr (f, m))
        for i in xrange (nre - nrf, nre) :
            l = self.__cuf2unf_readstr (f, m)
            self.add_event (l, iscutoff=True)

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
                if not e.pre :
                    raise Exception, 'Event e%d has empty preset' % e.nr

    def enables (self, m, e) :
        return e.pre | e.cont <= m

    def enabled (self, m) :
        s, u = set (), set ()
        for c in m : u |= set (c.post)
        for e in u :
            if self.enables (m, e) : s.add (e)
        assert (s == self.enabled2 (m))
        return s

    def enabled2 (self, m) :
        s = set ()
        for e in self.events[1:] :
            if self.enables (m, e) : s.add (e)
        return s

    def fire (self, run, m=None) :
        if m == None : m = self.m0.copy ()
        for e in run :
#            db ('at', list (m))
#            db ('firing', e)
            if not self.enables (m, e) :
                raise Exception, 'Cannot fire, event not enabled'
            m -= set (e.pre)
            m |= set (e.post)
#        db ('reached', list (m), 'enables', self.enabled (m))
        return m

    def run_of (self, conf) :
        # TODO: implement the topological sort by hand
        g = self.asym_graph (True, conf, True)
        return networkx.topological_sort (g)

#        # only for non-contextual nets:
#        run = []
#        m = self.m0.copy ()
#        while conf :
#            db ('at', list (m))
#            ena = list(self.enabled (m) & conf)
#            db ('enabled', ena)
#            if not ena :
#                raise Exception, 'Not a configuration'
#            run += ena
#            conf -= ena
#            db ('enabled', ena, 'remains', len (conf))
#            m = fire (ena, m)
#        return run

    def is_configuration (self, s) :
        for e in s :
            pre = set ([c.pre for c in (e.pre | e.cont)])
            pre.discard (None)
#            if not (pre <= s) : db ('event', e, 'not cc')
            if not (pre <= s) : return False
        g = self.asym_graph (True, s, True)
        b = networkx.is_directed_acyclic_graph (g)
#        if not b : db ('is not a dag')
        return b

    def marking_of (self, conf) :
        m = self.m0.copy ()
        for e in conf :
            m |= set (e.post)
        for e in conf :
            assert (set (e.pre) <= m)
            m -= set (e.pre)
        assert (self.is_configuration (conf))
        assert (m == self.fire (self.run_of (conf)))
        return m

    def asym_graph (self, symm=True, s=None, cutoffs=False) :
        # build the asymmetric conflict relaton
        g = networkx.DiGraph ()
        u = self.events[1:] if s == None else s
        for e in u :
            if not cutoffs and e.iscutoff : continue
            for c in e.pre :
                if c.pre : g.add_edge (c.pre, e, color=1)
                for ep in c.cont : self.__red_edge (g, ep, e, 2)
                if symm :
                    for ep in c.post :
                        if ep != e and (cutoffs or not ep.iscutoff) :
                            g.add_edge (ep, e, color=1)
                            g.add_edge (e, ep, color=1)
            for c in e.cont :
                if c.pre and (cutoffs or not c.pre.iscutoff):
                    g.add_edge (c.pre, e, color=1)

        if s != None : g = g.subgraph (s)
#        opt = ''
#        if symm : opt += 'with symm. confl. '
#        if s != None : opt += 'restricted to %d events ' % len (s)
#        if cutoffs : opt += 'with cutoffs!'
#        db ('nodes', len (g), 'edges', len (g.edges()), opt)

        return g

    def __red_edge (self, g, u, v, c) :
        g.add_edge (u, v)
        if not 'color' in g[u][v] or g[u][v]['color'] == 2 :
            g[u][v]['color'] = c

    def stats (self, f) :
        po = 0.0
        pre = 0.0
        co = 0.0
        poc = 0.0
        succlen = 0.0
        succ0 = 0.0
        succpre1 = 0.0
        succpre2 = 0.0
        for e in self.events[1:] :
            po += len (e.post)
            pre += len (e.pre)
            co += len (e.cont)
            if not e.iscutoff: poc += len (e.post)

            succ = set ()
            for c in e.post : succ |= c.post
            succ = set (e for e in succ if not e.iscutoff)
            succlen += len (succ)

            if len (succ) == 0 :
                succ0 += 1
            found = False
            for ep in succ :
                if len (ep.pre) != 1: found = True
            if not found : succpre1 += 1
            found = False
            for ep in succ :
                if len (ep.pre) > 2: found = True
            if not found : succpre2 += 1

        f.write ('event post %.3f\n' % (po / len (self.events[1:])))
        f.write ('event pre %.3f\n' % (pre / len (self.events[1:])))
        f.write ('event cont %.3f\n' % (co / len (self.events[1:])))
        f.write ('noncutoffevent post %.3f\n' % (poc / (len (self.events[1:]) - self.nr_cutoffs)))
        f.write ('event noncutoffsucc %.3f\n' % (succlen / len (self.events[1:])))
        f.write ('event noncutoffsucc=0 %.1f%%\n' % (succ0 * 100 / len (self.events[1:])))
        f.write ('event noncutoffsucc.pre=1 %.1f%%\n' % (succpre1 * 100 / len (self.events[1:])))
        f.write ('event noncutoffsucc.pre<=2 %.1f%%\n' % (succpre2 * 100 / len (self.events[1:])))
            
        po = 0.0
        po0 = 0.0
        po1 = 0.0
        poc = 0.0
        pre = 0.0
        co = 0.0
        for c in self.conds[1:] :
            po += len (c.post)
            if c.pre != None : pre += 1
            co += len (c.cont)
            poc += len ([e for e in c.post if not e.iscutoff])
            if len (c.post) == 0 : po0 += 1
            if len (c.post) == 1 : po1 += 1

        f.write ('cond pre %.3f\n' % (pre / len (self.conds[1:])))
        f.write ('cond cont %.3f\n' % (co / len (self.conds[1:])))
        f.write ('cond post %.3f\n' % (po / len (self.conds[1:])))
        f.write ('cond post (without cffs) %.3f\n' % (poc / len (self.conds[1:])))
        per0 = 100.0 * po0 / len (self.conds[1:])
        per1 = 100.0 * po1 / len (self.conds[1:])
        f.write ('cond post=0 %.1f%%\n' % per0)
        f.write ('cond post=1 %.1f%%\n' % per1)
        f.write ('cond post>1 %.1f%%\n' % (100.0 - per0 - per1))

        ca = 0.0
        for e in self.events[1:] :
            ca += sum ((1 if c.pre else 0) for c in e.pre | e.cont)
        f.write ('event causal %.3f\n' % (ca / len (self.events[1:])))
        ca = 0.0
        for e in self.events[1:] :
            ca += len (set ([c.pre for c in e.pre | e.cont if c.pre != None]))
        f.write ('event optmcausal %.3f\n' % (ca / len (self.events[1:])))

        pre = 0.0
        co = 0.0
        trans = set ()
        for e in self.events[1:] :
            if e.label in trans : continue
            trans.add (e.label)
            pre += len (e.pre)
            co += len (e.cont)
        f.write ('firable-trans pre %.3f\n' % (pre / len (trans)))
        f.write ('firable-trans cont %.3f\n' % (co / len (trans)))

        ste = 0
        stc = 0
        i = 0
        for e in self.events[1:] :
            if self.is_stubborn_empty (e) : ste += 1
            if self.is_stubborn_causal (e) : stc += 1
            i += 1
            f.write ('\r ... %d of %d' % (i, len (self.events)))
            f.flush ();

        ste = 100.0 * ste / (len (self.events) - 1)
        stc = 100.0 * stc / (len (self.events) - 1)
        f.write ('\nstubborn-empty %.3f\n' % ste)
        f.write ('stubborn-causal %.3f\n' % stc)
            
    def is_stubborn_empty (self, e) :
        for c in e.pre :
            if len (c.post) >= 2 : return False
        for c in e.cont :
            if len (c.post) != 0 : return False
        return True
    
    def is_stubborn_causal (self, e) :
        for c in e.pre :
            if len (c.post) >= 2 : return False
        ac = self.anti_cone (e)
        for c in e.cont :
            for ep in c.post :
                if not ep in ac : return False;
        return True

    def cone (self, e) :
        work = [e]
        cone = set (work)
        for e in work :
            for c in e.pre | e.cont :
                if c.pre and not c.pre in cone :
                    cone.add (c.pre)
                    work.append (c.pre)
        return cone

    def anti_cone (self, e) :
        work = [e]
        cone = set (work)
        for e in work :
            for c in e.post :
                for ep in c.cont | c.post :
                    if not ep in cone :
                        cone.add (ep)
                        work.append (ep)
        return cone

def test1 () :
    f = open ('/tmp/cesar/test.cuf', 'r')
    f1 = open ('/tmp/cesar/out.dot', 'w')
    f2 = open ('/tmp/cesar/out-c.dot', 'w')
    f3 = open ('/tmp/cesar/out-ce.dot', 'w')
    u = Unfolding (True)
    u.read (f, fmt='cuf')
    u.write (f1, 'dot')
    u.rem_cond (20)
    u.write (f2, 'dot')
    u.rem_event (16)
    u.write (f3, 'dot')
    del u

    for x in [f, f1, f2] : x.close ()

def test2 () :
    f = open ('/tmp/cesar/test.cuf', 'r')
    f1 = open ('/tmp/cesar/out.dot', 'w')

    u = Unfolding (True)
    u.read (f, fmt='cuf')

    items = set ([u.events[1]])

    u.write (f1, 'ctxdot', items, 3)

# vi:ts=4:sw=4:et:
