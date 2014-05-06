
/* 
 * Copyright (C) 2010, 2011  Cesar Rodriguez <cesar.rodriguez@lsv.ens-cachan.fr>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdexcept>

#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>

#include "cunf/global.h"
#include "cunf/output.h"
#include "cunf/netconv.h"
#include "cunf/readpep.h"
#include "cunf/unfold.h"
#include "cunf/debug.h"
#include "util/glue.h"
#include "util/misc.h"

#include "cna/speccheck.hh"

extern "C" {
/* all gloabal data is stored in this structure */
struct u u;
struct opt opt;
}

// FIXME mention Minisat

void help (void)
{
	const char * s = R"XXX(
The Cunf Toolset - a model checker for Petri nets (with read arcs)

Copyright (C) 2010-2014  Cesar Rodriguez <cesar.rodriguez@cs.ox.ac.uk>
Department of Computer Science, University of Oxford, UK

This program comes with ABSOLUTELY NO WARRANTY.  This is free software,
and you are welcome to redistribute it under certain conditions.  You should
have received a copy of the GNU General Public License along with this program.
If not, see <http://www.gnu.org/licenses/>.


Usage: cunf [OPTION]... NET [SPECIFICATION]

where NET is a path to an .ll_net input file and SPECIFICATION is an
optional file containing properties to verify.  Allowed OPTIONS are:

 -c RULE, --cutoff=RULE
      Sets the algorithm used for computing which events are the cutoff
      points of the prefix, as well the order to insert histories into the
      unfolding.  RULE can be one of the following:
       * mcm     McMillan's algorithm
       * parikh  Parikh vector strategy FIXME
       * erv     Esparza-Romer-Vogler's total order
       * mole    The modification of ERV implemented in the unfolder Mole

 --max-depth=N
 --max-events=N
 --max-conditions=N
 --max-histories=N
      Stop inserting histories (events) into the constructed unfolding
      prefix under different conditions.
      With --max-depth, Cunf will skip inserting histories whose depth is
      strictly larger than N (events enabled at the initial marking have a
      depth of 1).  Remaining options stop the algorithm whenever the
      indicated number of events, conditions, or histories has been
      reached.
      Observe that these options do not alter the order in which histories
      are inserted into the prefix. However, notice that --max-depth can
      unexpectedly interact with the cutoff algorith, and force to insert
      events that otherwise would have been left out.
       
 -s FILE, --save-unf=FILE
      Saves a copy of the unfolding to FILE.

 -i, --stats
      Print statistical information about the computation and the computed
      unfolding prefix.

 -v, --verb=N
      Increments the verbosity level by the optional parameter N, a number
      between 0 and 3, with 3 being the most verbose level).  If N is not
      provided it is assumed to be 1.  Multiple occurrences accumulate.

 -h, --help
      Shows this message.

 -V, --version
      Prints version information.

For more information, see https://code.google.com/p/cunf/
)XXX";

	printf("%s\nCompiled on %s\n", s, __DATE__);
	exit (EXIT_SUCCESS);
}

void usage (void)
{
	PRINT ("Usage: cunf [OPTION]... NET [SPECIFICATION]\n"
			"Try 'cunf --help' for more information.");
	exit (EXIT_FAILURE);
}

void version (void)
{
	PRINT ("Cunf v.FIXME");
	exit (EXIT_SUCCESS);
}

void res_usage (void)
{
	struct rusage r;
	char buff[128];
	int fd, ret;

	u.unf.usrtime = 0;
	u.unf.maxrss = 0;
	ret = getrusage (RUSAGE_SELF, &r);
	if (ret >= 0) {
		/* in linux this is 0; in mac os this is the maxrss in kb */
		u.unf.maxrss = r.ru_maxrss / 1024;
		u.unf.usrtime = r.ru_utime.tv_sec * 1000 +
				r.ru_utime.tv_usec / 1000;
	}

	/* this will only work in linux, in macos u.unf.maxrss is set to maxrss
	 * in kb */
	fd = open ("/proc/self/statm", O_RDONLY);
	if (fd < 0) return;
	ret = read (fd, buff, 128);
	close (fd);
	buff[127] = 0;
	u.unf.maxrss = strtoul (buff, 0, 10) * sysconf (_SC_PAGESIZE) >> 10;
}

char * peakmem (void)
{
	static char b[16] = "?";
	char buff[4096];
	char *s;
	int fd, ret;

	fd = open ("/proc/self/status", O_RDONLY);
	if (fd < 0) return b;

	ret = read (fd, buff, 4096);
	close (fd);
	if (ret >= 4096) {
		PRINT ("Bug in peakmem!!\n");
		exit (1);
	}
	buff[ret] = 0;
	s = strstr (buff, "VmPeak:\t");
	if (! s) return b;
	s[16] = 0;
	sprintf (b, "%u", atoi (s + 8));
	return b;
}

/*
 * Progress message:
 *
 * Events
 * Conditions
 * Ev/s
 * avg depth
 * max depth
 */

void stats ()
{

#ifdef CONFIG_DEBUG
	db_mem ();
#endif
	res_usage ();
	PRINT ( \
		"cpu time             : %.3f\n"
		"max memory (rss)     : %ld\n"

		"histories            : %d\n"
		"events               : %d\n"
		"conditions           : %d\n"

		"generating cond.     : %d\n"
		"reading cond.        : %d\n"
		"compound cond.       : %d\n"

		"vector r(h) [avg]    : %.2f\n"
		"vector s(h) [avg]    : %.2f\n"
		"vector co(r) [avg]   : %.2f\n"
		"vector rco(r) [avg]  : %.2f\n"
		"vector mrk(h) [avg]  : %.2f\n"

		"pre-set(e) [avg]     : %.2f\n"
		"context(e) [avg]     : %.2f\n"
		"post-set(e) [avg]    : %.2f\n"

		"cutoffs              : %d\n"
		"white events         : %llu\n"
		"gray events          : %llu\n"
		"black events         : %llu\n"
		"net                  : %s",

		u.unf.usrtime / 1000.0,
		u.unf.maxrss / 1024,

		u.unf.numh - 1,
		u.unf.numev - 1,
		u.unf.numcond,

		u.unf.numgen,
		u.unf.numread,
		u.unf.numcomp,

		u.unf.numr / (float) (u.unf.numh - 1),
		u.unf.nums / (float) (u.unf.numh - 1),
		u.unf.numco / 
			(float) (u.unf.numgen + u.unf.numread + u.unf.numcomp),
		u.unf.numrco / 
			(float) (u.unf.numgen + u.unf.numread + u.unf.numcomp),
		u.unf.nummrk / (float) (u.unf.numh - 1),

		u.unf.numepre / (float) (u.unf.numev - 1),
		u.unf.numecont / (float) (u.unf.numev - 1),
		u.unf.numepost / (float) (u.unf.numev - 1),


		u.unf.numcutoffs,
		u.unf.numewhite, // FIXME, these are available only after write_cuf!!
		u.unf.numegray,
		u.unf.numeblack,
		opt.net_path);
}

void parse_options (int argc, char ** argv)
{
	int op, verb;
	char *endptr;
	struct option longopts[] = {
			{"cutoff", required_argument, 0, 'c'},
			{"max-depth", required_argument, 0, 'd'},
			{"max-events", required_argument, 0, 'e'},
			{"max-conditions", required_argument, 0, 'k'},
			{"max-histories", required_argument, 0, 'H'},
			{"save-unf", required_argument, 0, 's'},
			{"stats", no_argument, 0, 'i'},
			{"help", no_argument, 0, 'h'},
			{"verb", optional_argument, 0, 'v'},
			{"version", no_argument, 0, 'V'},
			{0, 0, 0, 0}};

	// default options
	opt.save_path = 0;
	opt.cutoffs = OPT_ERV;
	opt.stats = 0;
	opt.maxdepth = INT_MAX;
	opt.maxev = INT_MAX;
	opt.maxcond = INT_MAX;
	opt.maxh = INT_MAX;
	verb = VERB_PRINT;

	// parse the command line, supress automatic error messages by getopt
	opterr = 0;
	while (1) {
		op = getopt_long (argc, argv, "c:s:ivhV", longopts, 0);
		if (op == -1) break;
		switch (op) {
		case 'c' :
			if (strcmp (optarg, "mcm") == 0) {
				opt.cutoffs = OPT_MCMILLAN;
			} else if (strcmp (optarg, "parikh") == 0) {
				opt.cutoffs = OPT_PARIKH;
			} else if (strcmp (optarg, "erv") == 0) {
				opt.cutoffs = OPT_ERV;
			} else if (strcmp (optarg, "mole") == 0) {
				opt.cutoffs = OPT_ERV_MOLE;
			} else {
				usage ();
			}
			break;
		case 'd' :
			opt.maxdepth = (int) strtol (optarg, &endptr, 10);
			if (optarg[0] == 0 || *endptr != 0) usage ();
			if (opt.maxdepth < 0) opt.maxdepth = INT_MAX;
			break;
		case 'e' :
			opt.maxev = (int) strtol (optarg, &endptr, 10);
			if (optarg[0] == 0 || *endptr != 0) usage ();
			if (opt.maxev < 0) opt.maxev = INT_MAX;
			break;
		case 'k' :
			opt.maxcond = (int) strtol (optarg, &endptr, 10);
			if (optarg[0] == 0 || *endptr != 0) usage ();
			if (opt.maxcond < 0) opt.maxcond = INT_MAX;
			break;
		case 'H' :
			opt.maxh = (int) strtol (optarg, &endptr, 10);
			if (optarg[0] == 0 || *endptr != 0) usage ();
			if (opt.maxh < 0) opt.maxh = INT_MAX;
			break;
		case 's' :
			opt.save_path = optarg;
			break;
		case 'i' :
			opt.stats = 1;
			break;
		case 'h' :
			help ();
		case 'v' :
			if (! optarg || optarg[0] == 0) { verb++; break; }
			verb += strtol (optarg, &endptr, 10);
			if (*endptr != 0) usage ();
			break;
		case 'V' :
			version ();
		default :
			usage ();
		}
	}

	if (optind == argc - 2) {
		opt.net_path = argv[optind];
		opt.spec_path = argv[optind + 1];
	} else if (optind == argc - 1) {
		opt.net_path = argv[optind];
		opt.spec_path = 0;
	} else {
		usage ();
	}

	if (verb < VERB_PRINT || verb > VERB_DEBUG) usage ();
	verb_set (verb);
}

void main_ (int argc, char **argv)
{
	cna::Speccheck verif;

	// parse commandline options
	parse_options (argc, argv);

	// welcome
	TRACE ("Net file: %s", opt.net_path);
	TRACE ("Specification file: %s",
			opt.spec_path ? opt.spec_path : "(none)");
	switch (opt.cutoffs)
	{
	case OPT_MCMILLAN :
		TRACE ("Cutoff algorithm: McMillan's");
		break;
	case OPT_PARIKH :
		TRACE ("Cutoff algorithm: Parikh strategy");
		break;
	case OPT_ERV :
		TRACE ("Cutoff algorithm: Esparza-Romer-Vogler");
		break;
	case OPT_ERV_MOLE :
		TRACE ("Cutoff algorithm: Mole's strategy");
		break;
	default :
		throw std::logic_error ("Bug in " __FILE__ );
	}
	if (opt.save_path)
		TRACE ("Saving the unfolding: Yes, to '%s'", opt.save_path);
	else
		TRACE ("Saving the unfolding: No");
	TRACE ("Verbosity level: %d\n", verb_get ());
	// FIXME print maxev, maxdepth, ...

	// initialize the unfolding structure
	u.mark = 2;
	u.stoptr = 0;

	// load the input net
	read_pep_net (opt.net_path);
	TRACE ("It is a *%s* net\n", u.net.isplain ? "plain" : "contextual");
	nc_static_checks ();

	// load the specification file, unfold, and do model checking
	if (opt.spec_path)
		verif.load_spec (opt.spec_path);
	unfold ();
	if (opt.spec_path)
		verif.verify ();

	// if requested, write the unfolding on disk and print statistics
	if (opt.save_path) {
		TRACE ("Writing unfolding to '%s'", opt.save_path);
		write_cuf (opt.save_path);
	}
	if (opt.stats) stats ();
}

int main (int argc, char **argv)
{
	try 
	{
		main_ (argc, argv);
		exit (EXIT_SUCCESS);
	}
	catch (std::exception & e)
	{
		PRINT ("Error: %s. Aborting.", e.what ());
	}
	catch (...)
	{
		PRINT ("An unknown error ocurred, can't tell more... Aborting.");
	}
	exit (EXIT_FAILURE);
}
