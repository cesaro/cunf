
'''Usage: trt [ARGS]
Where ARGS is an unordered list of pairs key=value.  The following keys are
available:
  t         Defines the name of the test (driver) to run.
  f         Comma-separated list of fields to print.
  timeout   Number of seconds before killing the test; -1 to run indefinitely.

For the 'cunf' test:
  net       Input .ll_net file to unfold.
  depth     For -d

For the 'mole' test:
  net       Input .ll_net file to unfold.

For the 'dl.mcm' test:
  mci       Input .mci file to check.

For the 'dl.smod' test:
  mci       Input .mci file to check.

For the 'dl.smv' test:
  net       Input .ll_net file to check.

For the 'dl.lola' test:
  net       Input .ll_net file to check.

For the 'dl.cnmc' test:
  cuf       Input .cuf file to check.

For the 'report' driver:
  in        Input file.  '-' for standard input.
  out       Output file.  '-' for standard output.
  f         Comma-separated list of column description.  Each column
            description is of the form test/field, where test is the name of
            a test and field is any field of that test.

It is mandatory to provide a value for the key 't'
'''

import os
import sys
import time
import signal
import select
import subprocess
import email.utils

def runit (args, timeout=-1) :
#    db (args, timeout)
    try :
        p = subprocess.Popen (args, bufsize=8192, stdin=subprocess.PIPE,
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                preexec_fn=os.setsid)
    except Exception :
        return ('noexec', '')
    
#    db ('pid', p.pid)
    try :
        killed = False
        s = ''
        p.stdin.close ()
        if timeout > 0 :
            tref = time.time ()
            while True :
                t = timeout - (time.time () - tref)
                if t <= 0 : t = 0
#                db ('select at', time.time () - tref, t)
                (r, w, x) = select.select ([p.stdout], [], [p.stdout], t)
#                db ('return at', time.time () - tref, r, w, x)
                if len (r) :
                    # reading only one is the only way to do it correctly
                    # since read (n) waits for n bytes before returning
                    c = p.stdout.read (1)
                    if len (c) == 0 : break
                    s += c
                else :
#                    db ('killing', p.pid)
                    os.killpg (p.pid, signal.SIGTERM)
                    killed = True
                    break
        p.wait ()
        s += p.stdout.read ()
        return (p.returncode if not killed else 'killed', s)
    except KeyboardInterrupt :
        os.killpg (p.pid, signal.SIGKILL)
        p.wait ()
        return ('ctrl+c', '')
    except :
        return ('fail', '')

class Cunf :
    EXT = '.unf.cuf'
    NAME = 'cunf'
    FIELDS = [ 'test', 'stat', 'time', 'mem', 'hist', 'events', 'cond',
    'noncff', 'gen', 'read', 'comp', 'r(h)', 's(h)', 'co(r)', 'rco(r)',
    'mrk(h)', 'pre(e)', 'ctx(e)', 'pst(e)', 'cutoffs', 'time2']

    @staticmethod
    def run (args) :
        if 'net' not in args : usage ("Parameter 'net' mandatory")
        cmd = ['src/main']
        if 'depth' in args : cmd += ['-d', args['depth']]
        cmd.append (args['net'])

        t = os.times () [2]
        (c, s) = runit (cmd, args['timeout'])
        t = os.times () [2] - t
        c = str (c)

        res = Trt.init_res (Cunf)
        res['test'] = Trt.subs_ext (args['net'], '.ll_net', Cunf.EXT)
        res['stat'] = c
        res['stdout'] = s
        res['time2'] = t
        if c != '0' : return res

        for l in s.splitlines () :
            (k, sep, v) = l.partition ('\t')
            if k in Cunf.FIELDS : res[k] = v
        return res

class Mole :
    EXT = '.unf.mci'
    NAME = 'mole'
    FIELDS = [ 'test', 'stat', 'time', 'events', 'cond']

    @staticmethod
    def run (args) :
        if 'net' not in args : usage ("Parameter 'net' mandatory")
        out = Trt.subs_ext (args['net'], '.ll_net', Mole.EXT)
        cmd = ['mole', args['net'], '-m', out]

        t = os.times () [2]
        (c, s) = runit (cmd, args['timeout'])
        t = os.times () [2] - t
        c = str (c)

        res = Trt.init_res (Mole)
        res['test'] = out
        res['stat'] = c
        res['stdout'] = s
        res['time'] = t
        if c != '0' : return res

        tup = s.split (' ')
        res['events'] = tup[1]
        res['cond'] = tup[3]
        return res

class Dlmcm :
    EXT = '.dl.mcm'
    NAME = 'dl.mcm'
    FIELDS = [ 'test', 'stat', 'result', 'time', 'seq', 'needed']

    @staticmethod
    def run (args) :
        if 'mci' not in args : usage ("Parameter 'mci' mandatory")
        out = Trt.subs_ext (args['mci'], '.unf.mci', Dlmcm.EXT)
        cmd = ['RdlcheckMcM', args['mci']]

        t = os.times () [2]
        (c, s) = runit (cmd, args['timeout'])
        t = os.times () [2] - t
        c = str (c)

        res = Trt.init_res (Dlmcm)
        res['test'] = out
        res['stat'] = c
        res['stdout'] = s
        res['time'] = t
        if c != '0' : return res

        lines = s.splitlines ()
        if len (lines) == 0 : return res
        if 'TRUE' in lines[0] :
            res['result'] = 'LIVE'
            res['seq'] = 'na'
        elif 'FALSE - EVERY' in lines[0] :
            res['result'] = 'DEAD-EVERY'
            res['needed'] = 'na'
            res['seq'] = 'na'
        elif 'FALSE' in lines[0] :
            res['result'] = 'DEAD'

        if res['result'] != 'DEAD-EVERY' :
            tup = lines[-1].split ()
            if len (tup) >= 3 : res['needed'] = tup[2]

        if res['result'] == 'DEAD' :
            seq = []
            for l in lines[1:-1] :
                for e in l.split (',') :
                    e = e.strip (' .\t')
                    if e : seq.append (e)
            if seq : res['seq'] = ' '.join (seq)
        return res

class Dlsmod :
    EXT = '.dl.smod'
    NAME = 'dl.smod'
    FIELDS = [ 'test', 'stat', 'result', 'seq', 'time', 'gen', 'solve', 'needed']

    @staticmethod
    def run (args) :
        if 'mci' not in args : usage ("Parameter 'mci' mandatory")
        out = Trt.subs_ext (args['mci'], '.unf.mci', Dlsmod.EXT)
        cmd = ['mcsmodels', '-v', args['mci']]

        t = os.times () [2]
        (c, s) = runit (cmd, args['timeout'])
        t = os.times () [2] - t
        c = str (c)

        res = Trt.init_res (Dlsmod)
        res['test'] = out
        res['stat'] = c
        res['stdout'] = s
        res['time'] = t
        if c != '0' : return res

        lines = s.splitlines ()
        if len (lines) == 0 : return res
        if 'TRUE' in lines[0] :
            res['result'] = 'LIVE'
            res['seq'] = 'na'
        elif 'FALSE - EVERY' in lines[0] :
            res['result'] = 'DEAD-EVERY'
            res['solve'] = 'na'
            res['gen'] = 'na'
            res['seq'] = 'na'
        elif 'FALSE' in lines[0] :
            res['result'] = 'DEAD'
            seq = []
            for l in lines[1:] :
                if not len (l) : break;
                for e in l.split (',') :
                    e = e.strip (' .\t')
                    if e : seq.append (e)
            if seq : res['seq'] = ' '.join (seq)

        for l in lines[1:] :
            if 'Time needed:' in l :
                tup = l.split ()
                if len (tup) >= 3 : res['needed'] = float (tup[2])
            if 'Duration:' in l :
                tup = l.split ()
                if len (tup) >= 2 : res['solve'] = float (tup[1])
                break;

        if type (res['needed']) == float and type (res['solve']) == float :
            res['gen'] = res['needed'] - res['solve']
        return res

class Dlclp :
    EXT = '.dl.clp'
    NAME = 'dl.clp'
    FIELDS = [ 'test', 'stat', 'result', 'seq', 'time', 'gen', 'solve']

    @staticmethod
    def run (args) :
        if 'mci' not in args : usage ("Parameter 'mci' mandatory")
        out = Trt.subs_ext (args['mci'], '.unf.mci', Dlclp.EXT)
        cmd = ['clp', args['mci']]

        t = os.times () [2]
        (c, s) = runit (cmd, args['timeout'])
        t = os.times () [2] - t
        c = str (c)

        res = Trt.init_res (Dlclp)
        res['test'] = out
        res['stat'] = c
        res['stdout'] = s
        res['time'] = t
        if c != '0' : return res

        lines = s.splitlines ()
        if len (lines) < 12 : return res
        if 'NO' == lines[10] :
            res['result'] = 'LIVE'
            res['seq'] = 'na'
        elif 'YES' == lines[10] :
            res['result'] = 'DEAD'
            if len (lines) >= 13 :
                res['seq'] = lines[12].replace (',', ' ')

        tup = lines[6].split ()
        if len (tup) >= 3 : res['gen'] = float (tup[1])
        tup = lines[8].split ()
        if len (tup) >= 3 : res['solve'] = float (tup[1])
        return res

class Dlsmv :
    EXT = '.dl.smv'
    NAME = 'dl.smv'
    FIELDS = [ 'test', 'stat', 'result', 'time', 'seq', 'needed']

    @staticmethod
    def run (args) :
        if 'net' not in args : usage ("Parameter 'net' mandatory")
        out = Trt.subs_ext (args['net'], '.ll_net', Dlsmv.EXT)
        cmd = ['check', 'pep:smv-dl', args['net']]

        t = os.times () [2]
        (c, s) = runit (cmd, args['timeout'])
        t = os.times () [2] - t
        c = str (c)

        res = Trt.init_res (Dlsmv)
        res['test'] = out
        res['stat'] = c
        res['stdout'] = s
        res['time'] = t
        if c != '0' : return res

        for l in s.splitlines () :
            if 'Time needed: ' in l :
                t = l.split ()
                if len (t) >= 3 : res['needed'] = float (t[2])
            elif 'Result: NO.' == l :
                res['result'] = 'DEAD'
            elif 'Result: YES.' == l :
                res['result'] = 'LIVE'
                res['seq'] = 'na'
            elif 'Counterexample: ' in l :
                res['seq'] = ' '.join (t.strip () for t in l[16:].split (','))
        return res

class Dllola :
    EXT = '.dl.lola'
    NAME = 'dl.lola'
    FIELDS = [ 'test', 'stat', 'result', 'time', 'seq', 'needed']

    @staticmethod
    def run (args) :
        if 'net' not in args : usage ("Parameter 'net' mandatory")
        out = Trt.subs_ext (args['net'], '.ll_net', Dllola.EXT)
        cmd = ['check', 'pep:lola-dl', args['net']]

        t = os.times () [2]
        (c, s) = runit (cmd, args['timeout'])
        t = os.times () [2] - t
        c = str (c)

        res = Trt.init_res (Dllola)
        res['test'] = out
        res['stat'] = c
        res['stdout'] = s
        res['time'] = t
        if c != '0' : return res

        for l in s.splitlines () :
            if 'Time needed: ' in l :
                t = l.split ()
                if len (t) >= 3 : res['needed'] = float (t[2])
            elif 'Result: NO.' == l :
                res['result'] = 'DEAD'
            elif 'Result: YES.' == l :
                res['result'] = 'LIVE'
                res['seq'] = 'na'
            elif 'Counterexample: ' in l :
                res['seq'] = ' '.join (t.strip () for t in l[16:].split (','))
            elif 'check: unclassified error in LoLA' == l :
                res['stat'] = 'error'
        return res

class Dlcnmc :
    EXT = '.dl.cnmc'
    NAME = 'dl.cnmc'
    FIELDS = [ 'test', 'stat', 'gen', 'solve', 'result', 'sym', 'asym', 'dis']

    @staticmethod
    def run (args) :
        if 'cuf' not in args : usage ("Parameter 'cuf' mandatory")
#        cmd = ['tools/cnmc.py', 'dl', 'conflicts=trans', 'symmetric=sub']
        cmd = ['tools/cnmc.py', 'dl', 'conflicts=trans']
        cmd.append (args['cuf'])

        t = os.times () [2]
        (c, s) = runit (cmd, args['timeout'])
        t = os.times () [2] - t
        c = str (c)

        res = Trt.init_res (Dlcnmc)
        res['test'] = Trt.subs_ext (args['cuf'], '.unf.cuf', Dlcnmc.EXT)
        res['stat'] = c
        res['stdout'] = s
        res['time'] = t
        if c != '0' : return res

        for l in s.splitlines () :
            (k, sep, v) = l.partition ('\t')
            if k in Dlcnmc.FIELDS : res[k] = v

        if res['result'] == 'DEAD-EVERY' :
             res['sym'] = 'na'
             res['asym'] = 'na'
             res['dis'] = 'na'

        return res

class Trt :
    TESTS = [Cunf, Mole, Dlmcm, Dlsmv, Dlsmod, Dlclp, Dllola, Dlcnmc]

    def __init__ (self) :
        self.byname = {}
        self.byext = {}

        for t in self.TESTS :
            self.byname[t.NAME] = t
            self.byext[t.EXT] = t
        self.byname['report'] = Trt

    @staticmethod
    def subs_ext (s, r, a) :
        if s[- len (r):] == r :
            s = s[:-len (r)]
        return s + a

    @staticmethod
    def part_ext (s, u) :
        f = ''
        for e in u :
            if s[- len (e):] == e :
                if len (e) > len (f) : f = e
        return (s[:- len (f)], f)

    @staticmethod
    def init_res (C) :
        d = {}
        for f in C.FIELDS : d[f] = '?'
        return d

    @staticmethod
    def output (k, v, fmt='%s') :
        if type (v) == float :
            fmt = '%.3f'
        print ('%s\t' + fmt) % (k, v)

    def run (self, args) :
        if args['t'] not in self.byname :
            usage ("Unknown test type '%s'" % args['t'])
        T = self.byname [args['t']]

        if args['t'] == 'report' : return self.report (args)

        if 'f' in args :
            fields = args['f'].split (',')
            fields = [f for f in fields if f and f != 'test' and f != 'stat']
            fields = ['test', 'stat'] + fields
            for f in fields :
                if f != 'stdout' and f not in T.FIELDS :
                    usage ("'%s' is not a field of test '%s'" % (f, T.NAME))
        else :
            fields = T.FIELDS

        res = T.run (args)
        for f in fields : Trt.output (f, res[f])

    def warn (self, f, l, m) :
        warn (f + ':' + str(l) + ': ' + m)

    def report (self, args) :

        # build the column specification of the report
        if not 'f' in args : usage ("Option 'f' is mandatory for 'report'")
        fields = args['f'].split (',')
        cols = []
        subcols = {}
        group = [None]
        for f in fields :
            (k, sep, v) = f.partition ('/')
#            db (k, sep, v)
            if sep and sep != '/' :
                usage ("'%s': Invalid column specification" % sep)
            if not k :
                usage ("Invalid column specification: missing test name")
            if k in self.byname :
                g = self.byname[k]
            else :
                usage ("'%s': unknown column name" % k)
            if group[0] != g :
                group = [g]
#                db ('new group', g.NAME)
                cols.append (group)
                if not g in subcols : subcols[g] = set ()
            if v :
                if v in g.FIELDS :
#                    db ('new col', v)
                    group.append (v)
                    subcols[g].add (v)
                else :
                    usage ("'%s': unknown column name" % v)
            else :
                l = [x for x in g.FIELDS if x != 'test']
#                db ('new col', l)
                group += l
                subcols[g] |= set (l)
        extensions = set (x.EXT for x in subcols)
#        db (cols)
#        db (subcols)
#        db (extensions)

        # open input and output file
        try :
            if not 'in' in args or args['in'] == '-' :
                fin = sys.stdin
                args['in'] = '<stdin>'
            else :
                fin = open (args['in'])
        except IOError as (e, m) :
            err ("'%s': %s" % (args['in'], m))
        except Exception, e:
            err ("'%s': %s" % (args['in'], e))

        try :
            if not 'out' in args or args['out'] == '-' :
                fout = sys.stdout
                args['out'] = '<stdout>'
            else :
                fout = open (args['out'], 'w+')
        except IOError as (e, m) :
            err ("'%s': %s" % (args['out'], m))
        except Exception, e:
            err ("'%s': %s" % (args['out'], e))
        
        # 'test\t' lines begin data from a test
        i = 0
        t = None
        case = None
        data = {}
        for line in fin :
            i += 1
            (k, sep, v) = line[:-1].partition ('\t')
            if '\t' in v :
                self.warn (args['in'], i, "more than one tab!!!")
            if k == 'test' :
                (case, ext) = Trt.part_ext (v, extensions)
#                db ('test', (case, ext))
                if ext in self.byext :
                    t = self.byext[ext]
                else :
                    self.warn (args['in'], i,
                            "'%s': test type not requested" % v)
                    t = None
            else :
                if t == None :
                    self.warn (args['in'], i, 'discarded')
                    continue
                if k in subcols[t] :
                    if case not in data : data[case] = {}
                    if t not in data[case] : data[case][t] = {}
                    data[case][t][k] = v
    
        for g in cols :
            fout.write (g[0].NAME)
            fout.write (''.join ('\t' for x in g[1:]))
        fout.write ('\n')
        for g in cols : fout.write ('\t'.join (g[1:]) + '\t')
        fout.write ('input\n')
        for case in sorted (data) :
            for g in cols :
                if not g[0] in data[case] : data[case][g[0]] = {}
                for k in g[1:] :
                    if not k in data[case][g[0]] :
                        data[case][g[0]][k] = 'missing'
                    fout.write (data[case][g[0]][k] + '\t')
            fout.write (case + '\n')

        s = '\nGenerated on ' + email.utils.formatdate (localtime=True)
        s += '; ' + str (len (data)) + ' test cases\n'
        fout.write (s)

        if args['in'] != '<stdin>' : fin.close ()
        if args['out'] != '<stdout>' : fout.close ()

def db (*msg) :
    s = ' '.join (str(x) for x in msg)
    sys.stderr.write ('trt: ' + s + '\n')

def usage (msg=None, code=1) :
    if msg :
        print msg + '\n'
    print __doc__
    sys.exit (code)

def err (msg, code=1) :
    if msg :
        sys.stdout.write (msg + '\n')
    sys.exit (code)

def warn (msg) :
        sys.stdout.write (msg + '\n')

def parse () :
    if len (sys.argv) == 1: usage (code = 0)
    args = {}
    for arg in sys.argv[1:] :
        (k, sep, v) = arg.partition ('=')
        args[k] = v
    if 't' not in args : usage ("No value for key 't'")
    if 'timeout' in args :
        args['timeout'] = float (args['timeout'])
    else :
        args['timeout'] = -1
    return args

def start () :
    args = parse ()
    trt = Trt ()
    trt.run (args)
    return 0

def main () :
    start ()
    return 0
    try :
        sys.exit (start ())
    except Exception, e :
        print 'trt:', e
        print >> sys.stderr, 'trt:', e

# vi:ts=4:sw=4:et:
