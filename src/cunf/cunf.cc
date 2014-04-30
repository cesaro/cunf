
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

#include "cna/cunfsat.hh"

extern "C" {
/* all gloabal data is stored in this structure */
struct u u;
struct opt opt;
}

/*****************************************************************************/

void help (void)
{
	const char * s = R"XXX(
The Cunf Toolset - a model checker for Petri nets (with read arcs)

Copyright (C) 2010-2014  Cesar Rodriguez <cesar.rodriguez@cs.ox.ac.uk>
Department of Computer Science, University of Oxford, UK

Uses code of ...
FIXME mention Minisat, Boost

This program comes with ABSOLUTELY NO WARRANTY.  This is free software,
and you are welcome to redistribute it under certain conditions.  You should
have received a copy of the GNU General Public License along with this program.
If not, see <http://www.gnu.org/licenses/>.


Usage: cunf [OPTION]... NET [SPECIFICATION]

where NET is a path to an .ll_net input file.  Allowed OPTIONS are:

 --cutoff={mcm,parikh,erv,mole,N}
      Sets the algorithm used for computing cutoff events:
       * mcm     McMillan's algorithm
       * parikh  Parikh vector strategy FIXME
       * erv     Esparza-Romer-Vogler's total order
       * mole    The modification of ERV implemented in the unfolder Mole
       * N       A non-negative number N will instruct Cunf to cut off
                 events of depth N; with N=0 the cutoff algorthm is
                 disabled and the unfolder may never stop

 --save-unf=FILE
      Saves a copy of the unfolding to FILE.

 -v, --verb=N
      Increments the verbosity level by the optional parameter N, a number
      between 0 and 3, with 3 being the most verbose level).  If N is not
      provided it is assumed to be 1.  Multiple occurrences allowed.

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
	u.unf.vmsize = 0;
	ret = getrusage (RUSAGE_SELF, &r);
	if (ret >= 0) {
		/* in linux this is 0; in mac os this is the maxrss in kb */
		u.unf.vmsize = r.ru_maxrss / 1024;
		u.unf.usrtime = r.ru_utime.tv_sec * 1000 +
				r.ru_utime.tv_usec / 1000;
	}

	/* this will only work in linux, in macos u.unf.vmsize is set to maxrss
	 * in kb */
	fd = open ("/proc/self/statm", O_RDONLY);
	if (fd < 0) return;
	ret = read (fd, buff, 128);
	close (fd);
	buff[127] = 0;
	u.unf.vmsize = strtoul (buff, 0, 10) * sysconf (_SC_PAGESIZE) >> 10;
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


#include "sat/cnf_minisat.hh"

void test (void)
{
	sat::Msat s;
	sat::Lit p, q, r;
	std::vector<sat::Lit> c(1);
	std::vector<sat::Lit> amo(3);

	p = s.new_var ();
	q = s.new_var ();
	r = s.new_var ();

	amo[0] = p;
	amo[1] = q;
	amo[2] = r;
	s.amo_2tree (amo);

	c[0] = p;
	s.add_clause (c);
	c[0] = q;
	s.add_clause (c);

	auto ret = s.solve ();
	if (ret == sat::Cnf::SAT) {
		DEBUG ("SAT");
		sat::CnfModel & m = s.get_model ();
		for (sat::Var v = 0; v < s.no_vars (); ++v)
		{
			DEBUG ("var %d is %s", v, m[v] ? "T" : "F");
		}
	} else if (ret == sat::Cnf::UNSAT) {
		DEBUG ("UNSAT");
	} else {
		DEBUG ("UNKNOWN");
	}
}

void parse_options (int argc, char ** argv)
{
	long int op, i;
	char *endptr;
	struct option longopts[] = {
			{"cutoff", required_argument, 0, 'c'},
			{"save-unf", required_argument, 0, 's'},
			{"help", no_argument, 0, 'h'},
			{"verb", optional_argument, 0, 'v'},
			{"version", no_argument, 0, '9'},
			{0, 0, 0, 0}};

	// default options
	opt.cutoffs = OPT_ERV;
	opt.save_path = 0;
	opt.depth = 0;
	i = VERB_PRINT;

	// parse the command line, supress automatic error messages by getopt
	opterr = 0;
	while (1) {
		op = getopt_long (argc, argv, "vhV", longopts, 0);
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
				opt.depth = strtol (optarg, &endptr, 10);
				if (optarg[0] == 0 || *endptr != 0) usage ();
				opt.cutoffs = OPT_DEPTH;
			}
			break;
		case 's' :
			opt.save_path = optarg;
			break;
		case 'v' :
			if (! optarg || optarg[0] == 0) { i++; break; }
			i += strtol (optarg, &endptr, 10);
			if (*endptr != 0) usage ();
			break;
		case 'h' :
			help ();
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

	if (i < VERB_PRINT || i > VERB_DEBUG) usage ();
	verb_set (i);
}

int main_ (int argc, char **argv)
{
	// parse commandline options
	parse_options (argc, argv);

	// welcome
	INFO ("Net file: '%s'", opt.net_path);
	if (opt.spec_path)
		INFO ("Specification file: '%s'", opt.spec_path);
	else
		INFO ("Specification file: (none)");
	switch (opt.cutoffs)
	{
	case OPT_MCMILLAN :
		INFO ("Cutoff algorithm: McMillan's");
		break;
	case OPT_PARIKH :
		INFO ("Cutoff algorithm: Parikh strategy");
		break;
	case OPT_ERV :
		INFO ("Cutoff algorithm: Esparza-Romer-Vogler");
		break;
	case OPT_ERV_MOLE :
		INFO ("Cutoff algorithm: Mole's strategy");
		break;
	case OPT_DEPTH :
		INFO ("Cutoff algorithm: Depth (%d)", opt.depth);
		break;
	default :
		throw std::logic_error ("Error while parsing the arguments");
	}
	if (opt.save_path)
		INFO ("Saving the unfolding: Yes, to '%s'", opt.save_path);
	else
		INFO ("Saving the unfolding: No");
	INFO ("Verbosity level: %d\n", verb_get ());

	// initialize the unfolding structure
	u.mark = 2;
	u.stoptr = 0;

	// load the input net
	read_pep_net (opt.net_path);
	TRACE ("It is a *%s* net\n", u.net.isplain ? "plain" : "contextual");
	nc_static_checks ();

	// build the unfolding
	INFO ("Unfolding ...");
	unfold ();
	
	// do model checking
	INFO ("Unfolding done. Analyzing ...");
	if (opt.spec_path) {
		cna::Cunfsat v;
		v.load_spec (std::string (opt.spec_path));
		v.encode ();
		bool ret = v.solve ();
		if (ret) {
			PRINT ("The net has a deadlock:");
			std::vector<struct event *> & conf = v.counterexample ();
			for (auto it = conf.begin (); it != conf.end (); ++it) db_e (*it);
		} else {
			PRINT ("The net has no deadlock");
		}
	}

	// if requested, write the unfolding on disk
	if (opt.save_path) {
		TRACE ("Writing unfolding to '%s'\n", opt.save_path);
		write_cuf (opt.save_path);
	}

#ifdef CONFIG_DEBUG
	db_mem ();
#endif
	rusage ();
	PRINT ("\ntime\t%.3f\n"
		"mem\t%ld\n"

		"hist\t%d\n"
		"events\t%d\n"
		"cond\t%d\n"

		"gen\t%d\n"
		"read\t%d\n"
		"comp\t%d\n"

		"r(h)\t%.2f\n"
		"s(h)\t%.2f\n"
		"co(r)\t%.2f\n"
		"rco(r)\t%.2f\n"
		"mrk(h)\t%.2f\n"

		"pre(e)\t%.2f\n"
		"ctx(e)\t%.2f\n"
		"pst(e)\t%.2f\n"

		"cutoffs\t%d\n"
		"ewhite\t%llu\n"
		"egray\t%llu\n"
		"eblack\t%llu\n"
		"net\t%s\n",

		u.unf.usrtime / 1000.0,
		u.unf.vmsize / 1024,

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
		u.unf.numewhite,
		u.unf.numegray,
		u.unf.numeblack,
		opt.net_path);

	// return EXIT_SUCCESS;
	exit (EXIT_SUCCESS);
}

int main (int argc, char **argv)
{
	try 
	{
		return main_ (argc, argv);
	}
	catch (std::exception & e)
	{
		PRINT ("Error: %s. Aborting.", e.what ());
	}
	catch (...)
	{
		PRINT ("An unknown error ocurred, can't tell more... Aborting.");
	}
}
