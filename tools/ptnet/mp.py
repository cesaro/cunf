import net

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
        for p in self.net.places :
            idxp[p] = i
            i += 1
            m = max (m, len (p.name))
        i = 1
        for t in self.net.trans :
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

            for c in e.pre | e.cont :
                f.write ('%d ' % idxc[c])
            for c in e.post | e.cont :
                f.write ('%d ' % idxc[c])
            f.write ('\n')
                
        f.write ('%d\n' % (m + 1))
        for p in self.net.places :
            f.write ('%s\n' % p.name)
        for t in self.net.trans :
            f.write ('%s\n' % t.name)

# vi:ts=4:sw=4:et:
