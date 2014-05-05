
import sys
import xml.parsers.expat

class Transition :
    def __init__ (self, name) :
        self.name = name
        self.pre = set ()
        self.cont = set ()
        self.post = set ()
        self.m = 0
        self.c = 0

    def __repr__ (self) :
        return str (self.name)

    def __str__ (self) :
        return "%s Pre %s;  Cont %s;  Post %s" \
                % (self.__repr__ (), self.pre, self.cont, self.post)

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

    def pre_rem (self, p) :
        if p not in self.pre : return
        self.pre.remove (p)
        p.post_rem (self)

    def cont_rem (self, p) :
        if p not in self.cont : return
        self.cont.remove (p)
        p.cont_rem (self)

    def post_rem (self, p) :
        if p not in self.post : return
        self.post.remove (p)
        p.pre_rem (self)

class Place :
    def __init__ (self, name, m0=0) :
        self.name = name
        self.pre = set ()
        self.cont = set ()
        self.post = set ()
        self.m0 = m0
        self.m = 0
        self.c = 0

    def __repr__ (self) :
        return str (self.name)

    def __str__ (self) :
        return "%s Pre %s;  Cont %s;  Post %s" \
                % (self.__repr__ (), self.pre, self.cont, self.post)

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

    def pre_rem (self, t) :
        if t not in self.pre : return
        self.pre.remove (t)
        t.post_rem (self)

    def cont_rem (self, t) :
        if t not in self.cont : return
        self.cont.remove (t)
        t.cont_rem (self)

    def post_rem (self, t) :
        if t not in self.post : return
        self.post.remove (t)
        t.pre_rem (self)

class Net :
    def __init__ (self, sanity_check=True) :
        self.places = []
        self.trans = []
        self.m0 = set ()
        self.sanity_check = sanity_check
        self.m = 1

        self.author = ''
        self.title = ''
        self.date = ''
        self.note = ''
        self.version = ''

    def place_add (self, name, m0=0) :
        p = Place (name, m0)
        self.places.append (p)
        if m0 : self.m0.add (p)
        # print 'place_add', name, m0
        return p

    def trans_add (self, name) :
        t = Transition (name)
        self.trans.append (t)
        # print 'trans_add', name
        return t

    def m0_add (self, p, n=1) :
        p.m0 = n
        self.m0.add (p)

    # limited support for firing executions in 1-safe nets
    def enables (self, m, t) :
        return t.pre | t.cont <= m

    def enabled (self, m) :
        s, u = set (), set ()
        for p in m : u |= set (p.post)
        for t in u :
            if self.enables (m, t) : s.add (t)
        assert (s == self.enabled2 (m))
        return s

    def enabled2 (self, m) :
        s = set ()
        for t in self.trans :
            if self.enables (m, t) : s.add (t)
        return s

    def fire (self, run, m=None) :
        if m == None : m = self.m0.copy ()
        for t in run :
#            db ('at', list (m))
#            db ('firing', t)
            if not self.enables (m, t) :
                raise Exception, 'Cannot fire, transition not enabled'
            m -= set (t.pre)
            m |= set (t.post)
#        db ('reached', list (m), 'enables', self.enabled (m))
        return m

    def plain2cont (self) :
        for t in self.trans :
            s = t.pre & t.post
            for p in s :
                t.pre_rem (p)
                t.post_rem (p)
                t.cont_add (p)

    def cont2plain (self) :
        for t in self.trans :
            for p in t.cont :
                t.pre_add (p)
                t.post_add (p)
            for p in list (t.cont) :
                t.cont_rem (p)

    def cont2pr (self) :
        pass

    def make_unsafe (self, places) :
        t1 = self.trans_add ('uns_t1')
        t2a = self.trans_add ('uns_t2a')
        t2b = self.trans_add ('uns_t2b')
        p2a = self.place_add ('uns_p2a')
        p2b = self.place_add ('uns_p2b')
        p3 = self.place_add ('uns_p3')
        t3 = self.trans_add ('uns_t3')

        for p in places :
            t1.pre_add (p)
        t1.post_add (p2a)
        t1.post_add (p2b)
        p2a.post_add (t2a)
        p2b.post_add (t2b)
        t2a.post_add (p3)
        t2b.post_add (p3)
        p3.post_add (t3)

    def write (self, f, fmt='pep', m=0) :
        if fmt == 'pep' : return self.__write_pep (f, m)
        if fmt == 'dot' : return self.__write_dot (f, m)
        if fmt == 'grml' : return self.__write_grml (f, m)
        if fmt == 'pnml' : return self.__write_pnml (f, m)
        if fmt == 'ctxdot' : return self.__write_ctxdot (f, 5, 1)
        raise Exception, "'%s': unknown output format" % fmt

    def __write_pep (self, f, m=0) :
        f.write ('PEP\nPetriBox\nFORMAT_N2\n')
        s = ''
        for att in ['author', 'title', 'date', 'note', 'version'] :
            if att in self.__dict__ :
                s += '% ' + att + ' "' + self.__dict__[att].strip () + '"\n'
        f.write (s + 'PL\n');

        tab = {'first' : 'element'}
        for p in self.places :
            if m != 0 and p.m != m : continue
            m1 = 'M%d' % p.m0 if p.m0 > 0 else ''
            f.write ('%d"%s"9@9%s\n' % (len (tab), repr (p), m1))
            tab[p] = len (tab)
        f.write ('TR\n')
        for t in self.trans :
            if m != 0 and t.m != m : continue
            f.write ('%d"%s"9@9\n' % (len (tab), repr (t)))
            tab[t] = len (tab)

        out = ''
        f.write ('TP\n')
        for t in self.trans :
            for p in t.post :
                if m != 0 and (t.m != m or p.m != m) : continue
                f.write ('%d<%d\n' % (tab[t], tab[p]))
            for p in t.cont :
                if m != 0 and (t.m != m or p.m != m) : continue
                out += '%d<%d\n' % (tab[t], tab[p])

        f.write ('PT\n')
        for t in self.trans :
            for p in t.pre :
                if m != 0 and (t.m != m or p.m != m) : continue
                f.write ('%d>%d\n' % (tab[p], tab[t]))

        if out : f.write ('RA\n' + out)

    # FIXME -- this method has never been tested !!
    def __write_ctxdot (self, f, items, n) :
        if len (items) != 0 : return self.__write_ctxdot_items (f, items, n)

        f.write ('digraph {\n')
        for t in self.trans :
            self.__write_ctxdot_items (f, set ([t]), n, repr (t), False)
        f.write ('}\n')

    # FIXME -- this method has never been tested !!
    # FIXME -- the line type (x.pre) will return Instance, not Event!
    def __write_ctxdot_items (self, f, items, n, prefx='', full=True) :
        self.m += 1
        m = self.m

        t = set (items)
        for x in t :
            x.m = m
            x.count = n
        while len (t) :
            s = t
            t = set ()
            for x in s :
                assert (x.m == m)
                if x.count <= 0 : continue

                for y in x.post | x.cont :
                    if y.m == m : continue
                    y.m = m
                    y.count = x.count - 1
                    t.add (y)
                if type (x.pre) == Event :
                    if x.pre.m != m :
                        x.pre.m = m
                        x.pre.count = x.count - 1
                        t.add (x.pre)
                elif type (x.pre) == set :
                    for y in x.pre :
                        if y.m == m : continue
                        y.m = m
                        y.count = x.count - 1
                        t.add (y)

        self.__write_dot (f, m, prefx, full)

    def __write_dot (self, f, m=0, prefx='', full=True) :
        if full : f.write ('digraph {\n')
        f.write ('\t/* transitions */\n')
        f.write ('\tnode\t[shape=box style=filled fillcolor=gray80];\n')
        for t in self.trans :
            if m != 0 and t.m != m : continue
            s = '\t%st%d [label="%s"' % (prefx, id (t), repr (t))
#            if e.isblack or e.isgray : s += ' shape=Msquare'
#            if (e.m) : s += ' fillcolor=blue'
            f.write (s + '];\n')

        f.write ('\n\t/* places, flow and context relations */\n')
        f.write ('\tnode\t[shape=circle fillcolor=gray95];')
        for p in self.places :
            if m != 0 and p.m != m: continue
            s = ' (%d)' % p.m0 if p.m0 > 0 else ''
            s = '\n\t%sp%d [label="%s%s"];\n' % (prefx, id (p), repr (p), s)
            for t in p.pre :
                if m == 0 or t.m == m :
                    s += '\t%st%d -> %sp%d;\n' % (prefx, id (t), prefx, id (p))

            for t in p.post :
                if m == 0 or t.m == m :
                    s += '\t%sp%d -> %st%d;\n' % (prefx, id (p), prefx, id (t))
            for t in p.cont :
                if m == 0 or t.m == m :
                    s += '\t%sp%d -> %st%d [arrowhead=none color=red];\n' \
                            % (prefx, id (p), prefx, id (t))
            f.write (s)

        if full :
            f.write ('\n\tgraph [label="%d transitions\\n%d places"];\n}\n' %
                    (len (self.trans), len (self.places)))

    def __write_pnml (self, f, m=0) :
        s = '<?xml version="1.0" encoding="UTF-8"?>\n'
        s += '<pnml xmlns="http://www.pnml.org/version-2009/grammar/pnml">\n'
        s += '<net id="n1" type="http://www.pnml.org/version-2009/grammar/ptnet">\n'
        s += '<name> <text>"%s" version "%s", by "%s"</text> </name>\n' % \
            (self.title, self.version, self.author)
        s += '<page id="page">\n'

        f.write (s + '\n<!-- places -->\n')
        tab = {}
        for p in self.places :
            if m != 0 and c.m != m : continue
            s = '<place id="p%d">\n' % len (tab)
            s += '<name><text>%s</text></name>\n' % repr (p)
            s += '<initialMarking> <text>%d</text> </initialMarking>\n' % p.m0
            s += '</place>\n'
            f.write (s)
            tab[p] = len (tab)

        f.write ('\n<!-- transitions -->\n')
        for t in self.trans :
            if m != 0 and t.m != m : continue
            s = '<transition id="t%d">\n' % len (tab)
            s += '<name><text>%s</text></name>\n' % repr (t)
            s += '</transition>\n'
            f.write (s)
            tab[t] = len (tab)

        f.write ('\n<!-- flow relation -->\n')
        for p in self.places :
            if m != 0 and c.m != m : continue
            s = ''
            for t in p.pre :
                if m == 0 or t.m == m :
                    s += '<arc id="a%d" source="t%d" target="p%d" />\n' \
                            % (len (tab), tab[t], tab[p])
                    tab[t, p, 'a'] = len (tab)
            for t in p.post :
                if m == 0 or t.m == m :
                    s += '<arc id="a%d" source="p%d" target="t%d" />\n' \
                            % (len (tab), tab[p], tab[t])
                    tab[p, t, 'a'] = len (tab)
            for t in p.cont :
                if m == 0 or t.m == m :
                    s += '<!-- read arc: -->\n'
                    s += ' <arc id="ra%d" source="p%d" target="t%d" />\n' \
                            % (len (tab), tab[p], tab[t])
                    tab[p, t, 'ra'] = len (tab)
                    s += ' <arc id="ra%d" source="t%d" target="p%d" />\n' \
                            % (len (tab), tab[t], tab[p])
                    tab[t, p, 'ra'] = len (tab)
            f.write (s)

        f.write ('\n</page>\n</net>\n</pnml>\n')


    def __write_grml (self, f, m=0) :
        s = '<?xml version="1.0" encoding="UTF-8"?>\n'
        s += '<model formalismUrl="http://formalisms.cosyverif.org/pt-net.fml"\n'
        s += '  xmlns="http://cosyverif.org/ns/model">\n\n'

        s += '<attribute name="authors">%s</attribute>\n' % self.author
        s += '<attribute name="title">%s</attribute>\n' % self.title
        s += '<attribute name="date">%s</attribute>\n' % self.date
        s += '<attribute name="version">%s</attribute>\n' % self.version
        s += '<attribute name="note">%s</attribute>\n\n' % self.note

        tab = {}
        f.write (s + '<!-- transitions -->\n')
        for t in self.trans :
            if m != 0 and t.m != m : continue
            s = '<node id="%d" nodeType="transition">\n' % len (tab)
            s += '<attribute name="name">%s</attribute>\n' % repr (t)
            s += '</node>\n'
            f.write (s)
            tab[t] = len (tab)

        f.write ('\n<!-- places, flow and context relations -->')
        for p in self.places :
            if m != 0 and c.m != m : continue
            s = '\n<node id="%d" nodeType="place">\n' % len (tab)
            s += '<attribute name="name">%s</attribute>\n' % repr (p)
            s += '<attribute name="marking">%d</attribute>\n' % p.m0
            s += '</node>\n'
            tab[p] = len (tab)

            for t in p.pre :
                if m == 0 or t.m == m :
                    s += '<arc id="%d" arcType="arc" source="%d" target="%d">\n' \
                            % (len (tab), tab[t], tab[p])
                    s += '<attribute name="valuation">1</attribute>\n'
                    s += '</arc>\n'
                    tab[t, p] = len (tab)
            for t in p.post :
                if m == 0 or t.m == m :
                    s += '<arc id="%d" arcType="arc" source="%d" target="%d">\n' \
                            % (len (tab), tab[p], tab[t])
                    s += '<attribute name="valuation">1</attribute>\n'
                    s += '</arc>\n'
                    tab[p, t] = len (tab)
            for t in p.cont :
                if m == 0 or t.m == m :
                    s += '<arc id="%d" arcType="readarc" source="%d" target="%d">\n' \
                            % (len (tab), tab[p], tab[t])
                    s += '<attribute name="valuation">1</attribute>\n'
                    s += '</arc>\n'
                    tab[p, t] = len (tab)
            f.write (s)
        f.write ('\n</model>\n')

    def read (self, f, fmt='pep') :
        if fmt == 'pep' : return self.__read_pep (f)
        if fmt == 'grml' : return self.__read_grml (f)
        if fmt == 'pnml' : return self.__read_pnml (f)
        if fmt == 'stg' : return self.__read_stg (f)
        raise Exception, "'%s': unknown input format" % fmt

    def __read_pep (self, f) :
        raise Exception, 'reading PEP files is not yet implemented'

    def __read_grml (self, f) :
        par = xml.parsers.expat.ParserCreate ()
        par.StartElementHandler = self.__grml_start
        par.EndElementHandler = self.__grml_end
        par.CharacterDataHandler = self.__grml_data

        self.__grmlq = []
        self.__grmlidx = {}
        par.ParseFile (f)
        del self.__grmlq
        del self.__grmlidx

    def __read_stg (self, f) :
        # TODO: signals of the form a+/4 ...
        nr = 0
        idx = {}
        for l in f :
            nr += 1
            l = l.strip ()
            if len (l) == 0 : continue
            if l[0] == '#' : continue
            if l[0:7] == '.inputs' :
                for s in l[7:].split () : idx[s] = None
            if l[0:8] == '.outputs' :
                for s in l[8:].split () : idx[s] = None
            if l[0:9] == '.internal' :
                for s in l[9:].split () : idx[s] = None
            if l[0:6] == '.dummy' :
                for s in l[6:].split () : idx[s] = self.trans_add (s)
            if l == '.graph' :
                for s in list (idx) :
                    idx[s + '+'] = self.trans_add (s + '+')
                    idx[s + '-'] = self.trans_add (s + '-')
            if l[0:8] == '.marking' :
                l = l[8:].strip (' {}')
                if '<' in l or '>' in l :
                    raise Exception, 'line %d: implicit places disallowed' % nr
                for s in l.split () :
                    if s not in idx :
                        raise Exception, 'line %d: place "%s" not found' % \
                                (nr, s)
                    if idx[s].__class__ != Place :
                        raise Exception, 'line %d: "%s" is not a place' % \
                                (nr, s)
                    idx[s].m0 = 1
                    self.m0.add (idx[s])
                continue
            if l[0] == '.' : continue

            ll = l.split ()
            if len (ll) < 2 :
                raise Exception, 'line %d: not a valid arc definition' % nr
            if ll[0] not in idx : idx[ll[0]] = self.place_add (ll[0])
            for n in ll[1:] :
                if n not in idx : idx[n] = self.place_add (n)
                if idx[ll[0]].__class__ == idx[n].__class__ :
                    raise Exception, 'line %d: not a p-t or t-p arc' % nr
                idx[ll[0]].post_add (idx[n])
                # print 'arc', idx[ll[0]], '->', idx[n]

    def __grml_start (self, tag, attr):
        # print "START", repr (tag), attr

        attr['__data'] = ''
        self.__grmlq.append (attr)
        if tag == 'model' :
            if not 'formalismUrl' in attr :
                raise Exception, "no 'formalismUrl' defined"
            if attr['formalismUrl'] != 'http://formalisms.cosyverif.org/pt-net.fml' :
                raise Exception, "unknown value for 'formalismUrl'"
        elif tag == 'arc' :
            attr['__type'] = attr['arcType']
        elif tag == 'node' :
            attr['__type'] = attr['nodeType']
        elif tag != 'attribute' :
                raise Exception, "'%s': unknown tag" % tag
        # print self.__grmlq

    def __grml_end (self, tag):
        # print "END", repr (tag)

        if tag == 'attribute' :
            if len (self.__grmlq) < 2 : raise Exception, 'Misplaced attribute'
            d = self.__grmlq.pop ()
            self.__grmlq[-1][d['name']] = d['__data']

        elif tag == 'node' :
            d = self.__grmlq.pop ()
            if 'id' not in d : raise Exception, 'id missing in node tag'
            if 'marking' not in d : d['marking'] = 0
            if 'name' not in d : d['name'] = ''
            if d['__type'] == 'place' :
                p = Place (d['name'], int (d['marking']))
                self.places.append (p)
                self.__grmlidx[d['id']] = p
                if p.m0 > 0 : self.m0.add (p)
            elif d['__type'] == 'transition' :
                t = Transition (d['name'])
                self.trans.append (t)
                self.__grmlidx[d['id']] = t
            else :
                raise Exception, "'%s': unknown nodeType" % d['__type']

        elif tag == 'arc' :
            d = self.__grmlq.pop ()
            if 'id' not in d or 'source' not in d or 'target' not in d :
                raise Exception, 'id, source or target missing in arc tag'
            if 'valuation' not in d : d['valuation'] = 1
            if d['valuation'] != '1' :
                raise Exception, 'Arc valuations different than 1 not supported'
            if d['source'] not in self.__grmlidx :
                raise Exception, 'node with id %d has not yet been seen' \
                        % d['source']
            if d['target'] not in self.__grmlidx :
                raise Exception, 'node with id %d has not yet been seen' \
                        % d['target']
            if d['__type'] == 'arc' :
                self.__grmlidx[d['source']].post_add (
                        self.__grmlidx[d['target']])
            elif d['__type'] == 'readarc' :
                self.__grmlidx[d['source']].cont_add (
                        self.__grmlidx[d['target']])
            else :
                raise Exception, "'%s': unknown arcType" % d['__type']

        elif tag == 'model' :
            d = self.__grmlq.pop ()
            if 'authors' in d : self.author = d['__data']
            if 'title' in d : self.title = d['__data']
            if 'date' in d : self.date = d['__data']
            if 'note' in d : self.note = d['__data']
            if 'version' in d : self.version = d['__data']
        else :
            raise Exception, 'Not a pt-net'

    def __grml_data (self, data):
        # print "DATA", repr (data)
        if len (self.__grmlq) : self.__grmlq[-1]['__data'] = data

    def __read_pnml (self, f) :
        par = xml.parsers.expat.ParserCreate ()
        par.StartElementHandler = self.__pnml_start
        par.EndElementHandler = self.__pnml_end
        par.CharacterDataHandler = self.__pnml_data

        self.__pnmlitm = {}
        self.__pnmlq = []
        par.ParseFile (f)
        if len (self.__pnmlitm) == 0 :
            raise Exception, 'missplaced "%s" entity' % tag
        self.__pnmlq.append (self.__pnmlitm)

        idx = {}
        for d in self.__pnmlq :
            if 'id' not in d : d['id'] = 'xxx'
            if 'name' not in d : d['name'] = d['id']
            if d['type'] == 'place' :
                if 'm0' not in d : d['m0'] = 0
                idx[d['id']] = self.place_add (d['name'], int (d['m0']))
            elif d['type'] == 'transition' :
                idx[d['id']] = self.trans_add (d['name'])
            elif d['type'] == 'net' :
                self.title = d['name']
        for d in self.__pnmlq :
            if d['type'] != 'arc' : continue
            if d['source'] not in idx or d['target'] not in idx :
                raise Exception, 'arc with unknown source or target'
            idx[d['source']].post_add (idx[d['target']])

        del self.__pnmlitm
        del self.__pnmlq

    def __pnml_start (self, tag, attr):
        #print "START", repr (tag), attr

        if tag == 'net' :
            if len (self.__pnmlitm) != 0 :
                raise Exception, 'missplaced "net" entity'
            self.__pnmlitm = {}
            self.__pnmlitm['type'] = 'net'

        elif tag in ['place', 'transition', 'arc'] :
            if len (self.__pnmlitm) == 0 :
                raise Exception, 'missplaced "%s" entity' % tag
            #print 'new! ', repr (self.__pnmlitm)
            for k in ['name', 'm0'] :
                if k in self.__pnmlitm :
                    self.__pnmlitm[k] = self.__pnmlitm[k].strip(' \n\t')
            self.__pnmlq.append (self.__pnmlitm)
            self.__pnmlitm = {}
            self.__pnmlitm['type'] = tag
            self.__pnmlitm['id'] = attr['id']
            for k in ['source', 'target'] :
                if k in attr :
                    self.__pnmlitm[k] = attr[k]

        elif tag == 'name' :
            self.__pnmlitm['data'] = 'name'
            self.__pnmlitm['name'] = ''
        elif tag == 'initialMarking' :
            self.__pnmlitm['data'] = 'm0'
            self.__pnmlitm['m0'] = ''

        elif tag in ['page', 'pnml', 'graphics', 'text', 'offset', 'text'] :
            return
        elif tag in ['position', 'inscription', 'dimension', 'fill', 'line'] :
            return
        else :
            # this else clause is just to be on the safe side
            raise Exception, 'unknown entity "%s"' % tag


    def __pnml_end (self, tag):
        #print "END  ", repr (tag)
        pass

    def __pnml_data (self, data):
        #data = data.strip(' \n\t') <- dangerous here, data can be split!!
        if len (data) == 0 : return

        #print "DATA ", repr (data)
        if 'data' not in self.__pnmlitm : return
        k = self.__pnmlitm['data']
        self.__pnmlitm[k] += data

def test1 () :
    n = Net (True)
    n.read (sys.stdin, 'grml')
    n.write (sys.stdout, 'grml')

if __name__ == '__main__' :
    test1 ()

# vi:ts=4:sw=4:et:
