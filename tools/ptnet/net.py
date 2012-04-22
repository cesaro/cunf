
class Transition :
    def __init__ (self, name) :
        self.name = name
        self.pre = set ()
        self.cont = set ()
        self.post = set ()
        self.m = 0
        self.c = 0

    def __repr__ (self) :
        return self.name

    def __str__ (self) :
        return "%s Pre %s;  Cont %s;  Post %s" \
                % (self.name, self.pre, self.cont, self.post)

    def pre_add (self, p) :
        if p in self.pre : return
        self.pre.add (p)
        p.post_add (self)

    def cont_add (self, p) :
        if p in self.cont : return
        self.cont.add (p)
        p.cont_add (self)

    def post_add (self, p) :
        if p in self.post : return
        self.post.add (p)
        p.pre_add (self)

class Place :
    def __init__ (self, name) :
        self.name = name
        self.pre = set ()
        self.cont = set ()
        self.post = set ()
        self.m0 = False
        self.m = 0
        self.c = 0

    def __repr__ (self) :
        return self.name

    def __str__ (self) :
        return "%s Pre %s;  Cont %s;  Post %s" \
                % (self.name, self.pre, self.cont, self.post)

    def pre_add (self, t) :
        if t in self.pre : return
        self.pre.add (t)
        t.post_add (self)

    def cont_add (self, t) :
        if t in self.cont : return
        self.cont.add (t)
        t.cont_add (self)

    def post_add (self, t) :
        if t in self.post : return
        self.post.add (t)
        t.pre_add (self)

class Net (object) :
    def __init__ (self, sanity_check=True) :
        self.places = set ()
        self.trans = set ()
        self.m0 = set ()
        self.sanity_check = sanity_check
        self.m = 1

    def write (self, f, fmt='pep') :
        if fmt == 'pep' : return self.__write_pep (f)
        raise Exception, "'%s': unknown output format" % fmt

    def __write_pep (self, f) :
        f.write ('PEP\nPetriBox\nFORMAT_N2\nPL\n')
        n = 1
        tab = {}
        for p in self.places :
            tab[p] = n
            m1 = 'M1m1' if p.m0 else ''
            f.write ('%d"%s"%s\n' % (n, p.name, m1))
            n += 1

        n = 1
        f.write ('TR\n')
        for t in self.trans :
            tab[t] = n
            f.write ('%d"%s"\n' % (n, t.name))
            n += 1

        out = ''
        f.write ('TP\n')
        for t in self.trans :
            for p in t.post :
                f.write ('%d<%d\n' % (tab[t], tab[p]))
            for p in t.cont :
                out += '%d<%d\n' % (tab[t], tab[p])

        f.write ('PT\n')
        for t in self.trans :
            for p in t.pre :
                f.write ('%d>%d\n' % (tab[p], tab[t]))

        if out : f.write ('RA\n' + out)

# vi:ts=4:sw=4:et:
