import net
import networkx

class Mpevent (net.Transition) :
    def __init__ (self, label, pre=set(), post=set(), cont=set(), cff=False) :
        net.Transition.__init__ (self, label)
        self.label = label
        self.iscutoff = cff

        for c in pre : self.pre_add (c)
        for c in post : self.post_add (c)
        for c in cont : self.cont_add (c)

    def __repr__ (self) :
        return ('*' if self.iscutoff else '') + repr (self.label)

class Mpcondition (net.Place) :
    def __init__ (self, place, depth, m0=0) :
        net.Place.__init__ (self, '%s/%d' % (place, depth), m0)
        self.label = place
        self.depth = depth

    def __repr__ (self) :
        return '%s/%d' % (repr (self.label), self.depth)

class Mprocess (net.Net) :
    def __init__ (self, sanity_check=True) :
        net.Net.__init__ (self, sanity_check)
        self.mpevents = self.trans
        self.mpconds = self.places
        self.nr_cutoffs = 0
        self.net = net.Net (sanity_check)

    def read (self, f, fmt='pep') :
        if fmt == 'mp' : return self.__read_mp (f)
        net.Net.read (self, f, fmt)

    def __read_mp (self, f) :
        nr = 1
        it = iter (f)
        try :
            line = next (it)
            l = line.split ()
            if len (l) != 4 : raise Exception
            nrmpconds = int (l[0])
            nrmpevents = int (l[1])
            nrplaces = int (l[2])
            nrtrans = int (l[3])
        except StopIteration :
            raise Exception, 'line 1: unexpected end of file'
        except :
            raise Exception, 'line 1: expected 4 integers'

        # create transitions and places of the net
        for i in range (nrplaces) :
            self.net.places.append (net.Place (''))
        for i in range (nrtrans) :
            self.net.trans.append (net.Transition (''))
        assert (nrplaces == len (self.net.places))

        # read mp-conditions
        for i in range (nrmpconds) :
            try :
                line = next (it)
                nr += 1
                l = line.split ()
                if len (l) != 3 :
                    raise Exception, 'line %d: expected three integers' % nr
                mpc  = Mpcondition (self.net.places[int (l[0]) - 1], int (l[1]), int (l[2]))
                self.mpconds.append (mpc)
                if mpc.m0 : self.m0.add (mpc)
            except StopIteration :
                raise Exception, 'line %d: unexpected end of file' % nr

        # read mp-events and flow relation
        for i in range (nrmpevents) :
            try :
                line = next (it)
                nr += 1
                l = line.split ()
                if len (l) < 4 :
                    raise Exception, 'line %d: expected at least 4 integers' % nr
                nrpre = int (l[2])
                nrpost = int (l[3])
                if len (l) != 4 + nrpre + nrpost :
                    raise Exception, 'line %d: expected %d integers' % (nr,
                            4 + nrpre + nrpost)
                mpe  = Mpevent (self.net.trans[int (l[0]) - 1], cff=(l[1] == '1'))
                for j in range (nrpre) :
                    mpe.pre_add (self.mpconds[int (l[4 + j])])
                for j in range (nrpost) :
                    mpe.post_add (self.mpconds[int (l[4 + nrpre + j])])
                self.mpevents.append (mpe)
                if mpe.iscutoff : self.nr_cutoffs += 1
            except StopIteration :
                raise Exception, 'line %d: unexpected end of file' % nr

        # skip maximal string length
        try :
            next (it)
            nr += 1
        except :
            pass
         
        # read place and transition names
        for i in range (nrplaces) :
            try :
                self.net.places[i].name = next (it).rstrip ()
                nr += 1
            except :
                raise Exception, 'line %d: unexpected end of file' % nr
        for i in range (nrtrans) :
            try :
                self.net.trans[i].name = next (it).rstrip ()
                nr += 1
            except :
                raise Exception, 'line %d: unexpected end of file' % nr
        
        # check there is no more lines
        try :
            next (it)
            nr += 1
            raise Exception, 'line %d: unexpected extra lines' % nr
        except StopIteration :
            pass

        # merge consume/produce loops into read arcs
        for e in self.mpevents :
            s = e.pre & e.post
            for c in s :
                e.pre_rem (c)
                e.post_rem (c)
                e.cont_add (c)

    def write (self, f, fmt='pep') :
        if fmt == 'mp' : return self.__write_mp (f)
        net.Net.write (self, f, fmt)

    def __write_mp (self, f) :

        f.write ('%d %d %d %d\n' % (len (self.mpconds), len (self.mpevents),
                len (self.net.places), len (self.net.trans)))

        # indexes to places and transitions start at 1; indexes to
        # mp-conditions start at 0
        idxp = {}
        idxt = {}
        idxc = {}
        i = 0
        for c in self.mpconds :
            idxc[c] = i
            i += 1
        i = 1
        m = 0
        out = ''
        for p in self.net.places :
            out += p.name + '\n'
            idxp[p] = i
            i += 1
            m = max (m, len (p.name))
        i = 1
        for t in self.net.trans :
            out += t.name + '\n'
            idxt[t] = i
            i += 1
            m = max (m, len (t.name))

        for c in self.mpconds :
            f.write ('%d %d %d\n' % (idxp[c.label], c.depth, c.m0))

        for e in self.mpevents :
            f.write ('%d %d %d %d ' % (idxt[e.label],
                1 if e.iscutoff else 0,
                len (e.pre) + len (e.cont),
                len (e.post) + len (e.cont)))

            i = len (e.pre) + len (e.cont) + len (e.post) + len (e.cont)
            for c in e.pre | e.cont :
                i -= 1
                f.write ('%d%s' % (idxc[c], ' ' if i else ''))
            for c in e.post | e.cont :
                i -= 1
                f.write ('%d%s' % (idxc[c], ' ' if i else ''))
            f.write ('\n')
                
        f.write ('%d\n%s' % (m + 1, out))

    def ac_graph (self) :
        g = networkx.DiGraph ()
        for e in self.mpevents :
            for c in e.post :
                g.add_edge (e, c, color='black')
            for c in e.pre :
                g.add_edge (c, e, color='black')
            for c in e.cont :
                g.add_edge (c, e, color='orange')
        for c in self.mpconds :
            if len (c.cont) and len (c.post) :
                v = (c, 'v')
                for e in c.cont :
                    g.add_edge (e, v, color='red')
                for e in c.post :
                    g.add_edge (v, e, color='red')
        return g

    def ac_dot (self, f) :
        g = self.ac_graph ()
        idx = {}
        col = {}

#        for n in g :
#            print repr (n)
#            for np in g[n] :
#                print '         ', repr (np), g[n][np], g[n][np]['color']
#                print g[n][np]['color']

        sccs = networkx.algorithms.strongly_connected_components (g)
        sccs = [x for x in sccs if len (x) >= 2]
        for s in sccs :
            print '/* scc', len (s), s, '*/'
            for n in s :
                col[n] = 'blue'
        i = 1
        for n in g :
            idx[n] = i
            i += 1

        f.write ('digraph {\n')
        f.write ('node [style=filled];\n')
        for n in g :
            color = col[n] if n in col else 'white'
            sha = ''
            if n in self.mpevents :
                sha = 'box'
            elif n in self.mpconds :
                sha = 'ellipse'
            else :
                sha = 'hexagon'

            f.write ('%d\t[fillcolor=%s shape=%s]\n' % (idx[n], color, sha))
        f.write ('\n\n')

        for n in g :
            for np in g[n] :
                f.write ('%d -> %d\t[color=%s]\n' % (
                        idx[n], idx[np], g[n][np]['color']))
        f.write ('}\n')

                    

# vi:ts=4:sw=4:et:
