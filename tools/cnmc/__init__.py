
'''Usage: cnmc CMD [OPTIONS] PATH
Where CMD is one of 
    deadlock|dl     Check for deadlocks in the model
and OPTIONS is
    (dl)  cnf       Generates a CNF formula instead of the boolean circuit
    (dl)  print     Print the circuit instead of invoking the sat solver on
                    it
    (dl)  assert    Verify that the deadlock is really a deadlock
    (dl)  conflicts States which encoding to use for conflicts. Legal values
                    are 'trans' (default), 'binary', 'cycle'
    (dl)  symmetric States which encoding to use for symmetric conflicts,
                    when the 'trans' encoding is used for conflicts.  Legal
                    values are 'all' (default), 'sub'
    (dl)  disabled  How to encode that no event is enabled in the model.
                    Legal values are 'all' (default), 'sub'

    (all) verbose   be more verbose
and PATH is the path to a .cuf file, or '-' for standard input
'''

import sys
import time
import cnmc

def db (*msg) :
    s = ' '.join (str(x) for x in msg)
    sys.stderr.write ('cnmc: ' + s + '\n')

def todot (g, f) :
    f.write ('digraph {\n')
    for (a, b) in g.edges () : f.write (repr (a) + ' -> ' + repr (b) + '\n')
    f.write ('}\n')

def usage (msg=None) :
    if msg :
        print msg + '\n'
    print __doc__
    sys.exit (1)

def parse () :
    cmd = {'dl' : 'dl', 'deadlock' : 'dl'}
    options = {'dl' : ['cnf', 'print', 'assert', 'verbose', 'conflicts', 'symmetric', 'disabled']}
    args = dict ()

    # at least two arguments, the command and the input file
    if len (sys.argv) < 3 : usage ()
    try :
        args['cmd'] = cmd[sys.argv[1]]
    except :
        usage ("Unknown command `%s\'" % sys.argv[1])
    args['path'] = sys.argv[-1]

    # fill the arguments with the expected options
    for arg in sys.argv[2:-1] :
        (k, sep, v) = arg.partition ('=')
        if not k in options[args['cmd']] :
            usage ("'%s' is not an option for command '%s'" % (k, args['cmd']))
        if sep == '' : v = True
        args[k] = v
#    db (args)
    return args

def output (k, v, fmt='%s') :
    print ('%s\t' + fmt) % (k, v)

def start () :
    args = parse ()
    mc = cnmc.Cnmc (args)

    if args['cmd'] == 'dl' :
        mc.deadlock ()

    l = list (mc.result)
    l.sort ()
    for k in l : output (k, mc.result[k])
    output ('input', args['path'])
    return 0

def main () :
    try :
        sys.exit (start ())
    except Exception, e :
        print 'cnmc:', e
    sys.exit (1)

# vi:ts=4:sw=4:et:
