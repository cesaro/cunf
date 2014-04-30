
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
	fprintf(stderr, "Branch eager v1.6, compiled %s\n", __DATE__); 
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

int main (int argc, char **argv)
{
	int op;
	char *endptr;
	struct option longopts[] = {
			{"cutoff", required_argument, 0, 'c'},
			{"save", required_argument, 0, 's'},
			{"help", no_argument, 0, 'h'},
			{"verb", optional_argument, 0, 'l'},
			{"version", no_argument, 0, 'v'},
			{0, 0, 0, 0}};

	// default options
	opt.cutoffs = OPT_ERV;
	opt.save_path = 0;
	opt.depth = 0;

	log_set_level (3);

	// parse the command line, supress automatic error messages by getopt
	opterr = 0;
	while (1) {
		op = getopt_long (argc, argv, "", longopts, 0);
		if (op == -1) break;
		switch (op) {
		case 'c' :
			if (strcmp (optarg, "mcm") == 0) {
				opt.cutoffs = OPT_MCMILLAN;
			} else if (strcmp (optarg, "erv") == 0) {
				opt.cutoffs = OPT_ERV;
			} else {
				opt.depth = strtol (optarg, &endptr, 10);
				if (optarg[0] == 0 || *endptr != 0) usage ();
				opt.cutoffs = OPT_DEPTH;
			}
			break;
		case 's' :
			opt.save_path = optarg;
			break;
		case 'l' :
			// FIXME, parse the number for the verbosity
			break;
		case 'h' :
			help ();
		case 'v' :
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

	INFO ("\nNet file: '%s'", opt.net_path);
	if (opt.spec_path)
		INFO ("Specification file: '%s'", opt.spec_path);
	else
		INFO ("Specification file: (none)");
	switch (opt.cutoffs)
	{
	case OPT_ERV :
		INFO ("Cutoff algorithm: Esparza-Romer-Vogler");
		break;
	case OPT_MCMILLAN :
		INFO ("Cutoff algorithm: McMillan's");
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

	// initialize the unfolding structure
	u.mark = 2;
	u.stoptr = 0;

	// load the input net
	TRACE ("");
	read_pep_net (opt.net_path);
	TRACE ("The net is a _%s_ net\n", u.net.isplain ? "plain" : "contextual");
	nc_static_checks ();

	// build the unfolding
	unfold ();
	
	log_set_level (3);
	// do model checking
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

		// if requested, write the unfolding on disk
		if (opt.save_path) {
			TRACE ("Writing unfolding to '%s'\n", opt.save_path);
			write_cuf (opt.save_path);
		}
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

	return EXIT_SUCCESS;
}
