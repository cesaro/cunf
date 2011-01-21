#!/usr/bin/env python

import math
import sys

# Series: 'sssss' (dd values)
# range : ddd -- ddd
# avg   : ddd,ddd
# var   : ddd,ddd
# stdev : ddd,ddd

def warn (s) :
	print s

def err (c, s) :
	if s : warn (s)
	exit (c)

def distrib () :
	d = {}
	series = []
	line = 1
	for s in sys.stdin :
		#print "line %d: '%s'" % (line, s)
		l = s.split ()
		if len (l) % 2 :
			err (1, 'line %d: odd number of fields' % line)
		i = 0
		while i < len (l) :
			f = l[i]
			try :
				v = float (l[i + 1])
				d["%s:nr" % f] += 1
				d["%s:sum" % f] += v
				d["%s:sqsum" % f] += v * v
				if v < d["%s:min" % f] : d["%s:min" % f] = v
				if v > d["%s:max" % f] : d["%s:max" % f] = v

			except KeyError :
				series.append (f)
				d["%s:nr" % f] = 1
				d["%s:sum" % f] = v
				d["%s:sqsum" % f] = v * v
				d["%s:min" % f] = v
				d["%s:max" % f] = v

			except ValueError :
				err (1, "line %d: '%s': not a number" % (line,
						l[i + 1]))
			i += 2
		line += 1

	for f in series :
		nr = d["%s:nr" % f]
		avg = d["%s:sum" % f] / d["%s:nr" % f]
		_min = d["%s:min" % f]
		_max = d["%s:max" % f]
		var = d["%s:sqsum" % f] / nr - avg * avg
		stdev = math.sqrt (var)

		print "Series: '%s' (%d values)" % (f, nr)
		print "range : %f -- %f" % (_min, _max)
		print "avg   : %f" % avg
		print "stdev : %f" % stdev
		print "var   : %f" % var
		print ""

if __name__ == "__main__" :
	distrib ()
