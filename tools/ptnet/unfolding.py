
import mp
import sys
import net
import struct
import socket
import networkx
import time

def sgl (s) :
    return (list (s))[0]

def db (*msg) :
    s = ' '.join (str(x) for x in msg)
    sys.stderr.write ('unfolding.py: ' + s + '\n')

def timeit (t = None, msg='') :
    if t == None :
        db ('### +0.000s', msg)
    else :
        db ('### +%.3fs' % (time.time () - t), msg)
    return time.time ()

class Event (net.Transition) :
    def __init__ (self, nr, label, pre=set(), post=set(), cont=set(),
            white=True, gray=False) :
        net.Transition.__init__ (self, nr)
        self.nr = nr
        self.label = label
        self.isblack = not white and not gray
        self.iswhite = white
        self.isgray = gray

        for c in pre : self.pre_add (c)
        for c in post : self.post_add (c)
        for c in cont : self.cont_add (c)

    def __repr__ (self) :
        if self.isblack :
            s = '*'
        elif self.isgray :
            s = '+'
        else :
            s = ''
        return '%s:%se%d' % (repr (self.label), s, self.nr)

class Condition (net.Place) :
    def __init__ (self, nr, label, pre=set(), post=set(), cont=set()) :
        net.Place.__init__ (self, nr)
        self.nr = nr
        self.label = label
        self.m0 = 1 if len (pre) == 0 else 0

        for e in pre : self.pre_add (e)
        for e in post : self.post_add (e)
        for e in cont : self.cont_add (e)

    def __repr__ (self) :
        return '%s:c%d' % (repr (self.label), self.nr)

class Unfolding (net.Net) :
    def __init__ (self, sanity_check=True) :
        net.Net.__init__ (self, sanity_check)
        self.conds = self.places
        self.events = self.trans
        self.nr_black = 0
        self.nr_gray = 0
        self.net = net.Net (sanity_check)

    def rem_cond (self, nr) :
        if self.sanity_check : self.__sane_cond_id (nr)
        c = self.conds[nr]
        if sgl (c.pre) == None :
            self.m0.discard (c)
        else :
            sgl (c.pre).post.remove (c)
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
        for c in e.post : c.pre = set ()
        if e.isblack : self.nr_black -= 1
        if e.isgray : self.nr_gray -= 1
        del e
        if nr != len (self.events) - 1 :
            db ('rem_event: ultimo antes del cambio', self.events[-1])
            for x in self.events[-1].pre : db ('rem_event: pre del ultimo antes', x)
            self.events[nr] = self.events[-1]
            self.events[nr].nr = nr
            db ('rem_event: ultimo despues del cambio', self.events[-1])
            for x in self.events[-1].pre : db ('rem_event: pre del ultimo despues', x)
        del self.events[-1]

    def read (self, f, fmt='cuf3') :
        if fmt == 'cuf' : fmt = 'cuf3'
        if fmt == 'cuf3' : return self.__read_cuf3 (f)
        net.Net.read (self, f, fmt)

    def __check_event_id (self, i) :
        if 0 <= i < len (self.events) : return
        raise Exception, '%s: invalid event nr' % i
    def __check_cond_id (self, i) :
        if 0 <= i < len (self.conds) : return
        raise Exception, '%s: invalid condition nr' % i
    def __check_place_id (self, i) :
        if 0 <= i < len (self.net.places) : return
        raise Exception, '%s: invalid place nr' % i
    def __check_trans_id (self, i) :
        if 0 <= i < len (self.net.trans) : return
        raise Exception, '%s: invalid transition nr' % i

    def __cuf2unf_readint (self, f) :
        s = f.read (4)
        if (len (s) != 4) : raise Exception, 'Corrupted CUF file'
        tup = struct.unpack ('I', s)
        return socket.ntohl (tup[0])

    def __read_cuf3 (self, f) :
        mag = self.__cuf2unf_readint (f)
        if mag != 0x43554603 : raise Exception, 'Not a CUF03 file'

        # read first seven fields
        nrp = self.__cuf2unf_readint (f)
        nrt = self.__cuf2unf_readint (f)
        nrc = self.__cuf2unf_readint (f)
        nre = self.__cuf2unf_readint (f)
        nrw = self.__cuf2unf_readint (f)
        nrg = self.__cuf2unf_readint (f)
        m = 1 + self.__cuf2unf_readint (f)
        # db (nrp, nrt, nrc, nre, nrw, nrg, m)

        # create nrp places and nrt transitions in self.net
        for i in xrange (nrp) :
            self.net.places.append (net.Place (None))
        for i in xrange (nrt) :
            self.net.trans.append (net.Transition (None))

        # read nre events
        for i in xrange (nrw) :
            idx = self.__cuf2unf_readint (f)
            self.__check_trans_id (idx)
            e = Event (len (self.events), self.net.trans[idx],
                    white=True, gray=False)
            # db ('event', len (self.events), 'trans idx', idx, 'white')
            self.events.append (e)
        for i in xrange (nrg) :
            idx = self.__cuf2unf_readint (f)
            self.__check_trans_id (idx)
            e = Event (len (self.events), self.net.trans[idx],
                    white=False, gray=True)
            # db ('event', len (self.events), 'trans idx', idx, 'gray')
            self.events.append (e)
        for i in xrange (nre - nrg - nrw) :
            idx = self.__cuf2unf_readint (f)
            self.__check_trans_id (idx)
            e = Event (len (self.events), self.net.trans[idx],
                    white=False, gray=False)
            # db ('event', len (self.events), 'trans idx', idx, 'black')
            self.events.append (e)
        self.nr_gray = nrg
        self.nr_black = nre - nrg - nrw

        # read condition labels, flow and context relations
        for i in xrange (nrc) :
            idx = self.__cuf2unf_readint (f)
            self.__check_place_id (idx)
            p = self.net.places[idx]
            # db ('cond', len (self.conds), 'place idx', idx)

            idx = self.__cuf2unf_readint (f)
            pre = set ()
            if idx != 0xffffffff :
                self.__check_event_id (idx)
                pre.add (self.events[idx])

            pos = self.__cuf2unf_readint (f)
            cos = self.__cuf2unf_readint (f)
            post = set ()
            cont = set ()
            for i in xrange (pos) :
                idx = self.__cuf2unf_readint (f)
                self.__check_event_id (idx)
                post.add (self.events[idx])
            for i in xrange (cos) :
                idx = self.__cuf2unf_readint (f)
                self.__check_event_id (idx)
                cont.add (self.events[idx])
            c = Condition (len (self.conds), p, pre, post, cont)
            self.conds.append (c)
            if c.m0 > 0 : self.m0.add (c)

        # finally, read transition and place names
        s = f.read ()
        if s[-1] != '\0' : raise Exception, 'Corrupted CUF file'
        s = s[:-1]
        l = s.split ('\0')
        if len (l) != nrt + nrp : raise Exception, 'Corrupted CUF file'
        for i in xrange (nrt) :
            self.net.trans[i].name = l[i]
            # db ('trans idx', i, 'name', l[i])
        for i in xrange (nrp) :
            self.net.places[i].name = l[nrt + i]
            # db ('place idx', i, 'name', l[nrt + i])

        # db (self.__dict__)

        if self.sanity_check :
            for e in self.events :
                if not e.pre and not e.cont :
                    raise Exception, 'Event %s has empty preset+context' \
                            % repr (e)

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
        pre = set ()
        for e in s :
            for c in e.pre | e.cont :
                if not sgl (c.pre) <= s : return False

        g = self.asym_graph (True, s, True)
        return networkx.is_directed_acyclic_graph (g)

    def marking_of (self, conf) :
        m = self.m0.copy ()
        for e in conf :
            m |= e.post
        for e in conf :
            assert (e.pre <= m)
            m -= e.pre
        assert (self.is_configuration (conf))
        assert (m == self.fire (self.run_of (conf)))
        return m

    def asym_graph (self, symm=True, s=None, cutoffs=False) :
        # build the asymmetric conflict relaton
        g = networkx.DiGraph ()
        u = self.events if s == None else s
        for e in u :
            if not cutoffs and e.isblack : continue
            for c in e.pre :
                if c.pre : g.add_edge (sgl (c.pre), e, color=1)
                for ep in c.cont : self.__red_edge (g, ep, e, 2)
                if symm :
                    for ep in c.post :
                        if ep != e and (cutoffs or not ep.isblack) :
                            g.add_edge (ep, e, color=1)
                            g.add_edge (e, ep, color=1)
            for c in e.cont :
                if c.pre :
                    ep = sgl (c.pre)
                    if cutoffs or not ep.isblack :
                        g.add_edge (ep, e, color=1)

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

    def __ic_graph (self) :
        g = networkx.DiGraph ()
        for c in self.conds :
            g.add_node (c)
            if len (c.pre) :
                g.add_edge (list (c.pre)[0], c)
                for e in c.cont : g.add_edge (list (c.pre)[0], e)
            for e in c.post : g.add_edge (c, e)
        for n in g : g.node[n]['depth'] = {}
        return g

    def __merge_depths (self, g) :
        # set up the list of labels whose depth each node has to track
        l = networkx.topological_sort (g)
        depths = {}
        t = time.time () # timeit
        for n in reversed (l) :
            nd = g.node[n]['depth']
            if isinstance (n, Condition) :
                nd[n.label] = 0
            for np in g[n] :
                for label in g.node[np]['depth'] :
                    nd[label] = 0
        t = timeit (t, 'computing required places')
        for n in l :
            nd = g.node[n]['depth']
            for np in g.predecessors (n) :
                npd = g.node[np]['depth']
                for label in nd :
                    d = npd[label]
                    if d > nd[label] : nd[label] = d
            if isinstance (n, Condition) :
                nd[n.label] += 1
                depths[n] = nd[n.label] 
        #db ('depthssssssssss', depths)
        timeit (t, 'computing max')
        return depths

    def merge (self) :

        # compute the immediate causality graph and condition depths
        t = timeit (None, 'Starting')
        g = self.__ic_graph ()
        t = timeit (t, 'building ic graph')
        d = self.__merge_depths (g)
        t = timeit (t, 'computing depths')

        # create the merged process and merge conditions (step 1)
        mproc = mp.Mprocess (self.sanity_check)
        mproc.net = self.net
        mpconds = {}
        for c in self.conds :
            if (c.label, d[c]) not in mpconds :
                #db ('new mp-cond', c.label, d[c])
                mpc = mp.Mpcondition (c.label, d[c], 0)
                mproc.mpconds.append (mpc)
                mpconds[c.label, d[c]] = mpc
            # warning! we dynamically create the field mpcond in Condition
            c.mpcond = mpconds[c.label, d[c]]
            #db ('condition', c, 'maps to mp-condition', c.mpcond)

        # for all initial conditions, make associated mp-conditions initial
        for c in self.m0 :
            c.mpcond.m0 += c.m0
            mproc.m0.add (c.mpcond)
        t = timeit (t, 'merging conditions (step 1)')

        # merge events (step 2)
        mpevs = {}
        for e in self.events :
            pre = frozenset (c.mpcond for c in e.pre)
            cont = frozenset (c.mpcond for c in e.cont)
            post = frozenset (c.mpcond for c in e.post)
            tup = (e.label, pre, cont, post)
            if tup not in mpevs :
                mpe = mp.Mpevent (e.label, pre, post, cont, e.isblack)
                #db ('new mp-event', mpe.__dict__)
                mproc.mpevents.append (mpe)
                mpevs[tup] = mpe
            # warning! we dynamically create a new field in Event !!
            e.mpev = mpevs[tup]
            if not e.isblack : mpevs[tup].iscutoff = False
            #db ('event', e, 'maps to mp-event', e.mpev)
        for mpe in mproc.mpevents :
            if mpe.iscutoff : mproc.nr_cutoffs += 1
        t = timeit (t, 'merging events (step 2)')

        ## cesar test
        #revmap = {}
        #for c in self.conds :
        #    if c.mpcond not in revmap :
        #        revmap[c.mpcond] = []
        #    revmap[c.mpcond].append (c)
        #for mpc in revmap :
        #    print 'len', len (revmap[mpc]), 'od', mpc.depth, 'list', revmap[mpc]
        #print
        #revmap = {}
        #for e in self.events :
        #    if e.mpev not in revmap :
        #        revmap[c.mpcond] = []
        #    revmap[c.mpcond].append (c)
        #for mpc in revmap :
        #    print 'len', len (revmap[mpc]), 'od', mpc.depth, 'list', revmap[mpc]
        #print
        return mproc

    def stats_print (f, k, v, fmt='%s') :
        f.write (fmt + '\t%s\n') % (v, k)

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
            if not e.isblack: wgpo += len (e.post)

            succ = set ()
            for c in e.post : succ |= c.post
            succ = set (e for e in succ if not e.isblack)
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

        self.stats_print (f, po / len (self.events[1:]), 'avg(post(e))')
        self.stats_print (f, pre / len (self.events[1:]), 'avg(pre(e))')
        self.stats_print (f, co / len (self.events[1:]), 'avg(cont(e))')
        self.stats_print (f, wgpo / (len (self.events[1:]) - self.nr_black), 'avg(post(e)) for non-black e')
        f.write ('event nonblacksucc %.3f\n' % (succlen / len (self.events[1:])))
        f.write ('event nonblacksucc=0 %.1f%%\n' % (succ0 * 100 / len (self.events[1:])))
        f.write ('event nonblacksucc.pre=1 %.1f%%\n' % (succpre1 * 100 / len (self.events[1:])))
        f.write ('event nonblacksucc.pre<=2 %.1f%%\n' % (succpre2 * 100 / len (self.events[1:])))
            
        po = 0.0
        pre = 0.0
        co = 0.0
        po0 = 0.0
        po1 = 0.0
        wgpo = 0.0
        top10 = []
        for c in self.conds[1:] :
            po += len (c.post)
            if c.pre != None : pre += 1
            co += len (c.cont)
            k = len ([e for e in c.post if not e.isblack])
            wgpo += k
            top10.append ((k, len (c.post), c))
            if len (c.post) == 0 : po0 += 1
            if len (c.post) == 1 : po1 += 1

        self.stats_print (f, pre / len (self.conds[1:]), 'avg(pre(c))')
        self.stats_print (f, po / len (self.conds[1:]), 'avg(post(c))')
        self.stats_print (f, co / len (self.conds[1:]), 'avg(cont(c))')
        self.stats_print (f, poc / len (self.conds[1:]), 'avg(cont(c))')

        f.write ('cond post (without cffs) %.3f\n' % (poc / len (self.conds[1:])))
        per0 = 100.0 * po0 / len (self.conds[1:])
        per1 = 100.0 * po1 / len (self.conds[1:])
        f.write ('cond post=0 %.1f%%\n' % per0)
        f.write ('cond post=1 %.1f%%\n' % per1)
        f.write ('cond post>1 %.1f%%\n' % (100.0 - per0 - per1))
        top10.sort ()
        top10.reverse ()
        del top10[40:]
        f.write ('cond post top10 (postw/black, post, e): %s\n' % top10)

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
                if c.pre and not sgl (c.pre) in cone :
                    cone.add (sgl (c.pre))
                    work.append (sgl (c.pre))
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

def test3 () :
    u = Unfolding (True)
    u.read (sys.stdin, fmt='cuf')
    #u.write (sys.stdout, fmt='dot')
    u.write (sys.stdout, fmt='pep')
    sys.exit (0)

def test4 () :
    u = Unfolding (True)
    f = open ('/tmp/depths.unf.cuf', 'r')
    fout = open ('/tmp/depths.mp.ll_net', 'w')
    u.read (f, fmt='cuf')
    mp = u.merge ()
    mp.write (fout, 'pep')
    sys.exit (0)

if __name__ == '__main__' :
    test3 ()

# vi:ts=4:sw=4:et:
