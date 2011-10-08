
import sys
import net
import networkx

def db (*msg) :
    s = ' '.join (str(x) for x in msg)
    sys.stderr.write ('cnmc: ' + s + '\n')

class Event (object) :
    def __init__ (self, nr, l="", pre=set(), post=set(), cont=set(), iscutoff=False) :
        self.nr = nr
        self.label = l
        self.pre = pre
        self.post = post
        self.cont = cont
        self.iscutoff = iscutoff
        self.mark = 0

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

    def write (self, f, fmt='dot') :
        if fmt != 'dot' : raise Exception, "'%s': unknown output format" % fmt
        f.write ('digraph {\n')
        f.write ('\t/* events */\n')
        f.write ('\tnode\t[shape=box style=filled fillcolor=grey80];\n')
        for e in self.events :
            if (e == None) : continue
            s = '\te%-6d [label="%s:e%d"' % \
                    (e.nr, e.label, e.nr)
            if (e.iscutoff) : s += ' shape=Msquare'
            if (e.mark) : s += ' fillcolor=blue'
            f.write (s + '];\n')

        f.write ('\n\t/* conditions, flow and context relations */\n')
        f.write ('\tnode\t[shape=circle fillcolor=gray95];')
        for c in self.conds :
            if (c == None) : continue
            s = '\n\tc%-6d [label="%s:c%d"];' % (c.nr, c.label, c.nr)
            if (c.pre == None) : s += ' /* initial */\n'
            else : s += '\n\te%-6d -> c%d;\n' % (c.pre.nr, c.nr)

            for e in c.post :
                s += '\tc%-6d -> e%d;\n' % (c.nr, e.nr)
            for e in c.cont :
                s += '\tc%-6d -> e%d [arrowhead=none color=red];\n' \
                        % (c.nr, e.nr)
            f.write (s)
        f.write ('}\n') 

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
        for e in self.events :
            if not e : continue
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
#        db ('reached', list (m))
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
                if c.pre : g.add_edge (c.pre, e)
                for ep in c.cont : g.add_edge (ep, e)
                if symm :
                    for ep in c.post :
                        if ep != e and (cutoffs or not ep.iscutoff) :
                            g.add_edge (ep, e)
                            g.add_edge (e, ep)
            for c in e.cont :
                if c.pre and (cutoffs or not c.pre.iscutoff):
                    g.add_edge (c.pre, e)

        if s != None : g = g.subgraph (s)
        opt = ''
        if symm : opt += 'with symm. confl. '
        if s != None : opt += 'restricted to %d events ' % len (s)
        if cutoffs : opt += 'with cutoffs!'
        db ('nodes', len (g), 'edges', len (g.edges()), opt)

        return g



# vi:ts=4:sw=4:et:
