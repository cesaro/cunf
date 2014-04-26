
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

extern "C" {
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>

#include "glue.h"
#include "debug.h"
#include "global.h"
#include "output.h"
#include "netconv.h"
#include "readpep.h"
#include "unfold.h"
}

#include "net/net.hh"

extern "C" {
/* all gloabal data is stored in this structure */
struct u u;
struct opt opt;
}

/*****************************************************************************/

void usage (void)
{
	fprintf(stderr,
"\n"
"The cunf tool -- a low level contextual Petri net unfolder\n"
"\n"
"Copyright (C) 2010-2012  Cesar Rodriguez <cesar.rodriguez@lsv.ens-cachan.fr>\n"
"Laboratoire de Specification et Verification (LSV), ENS Cachan, France\n"
"\n"
"This program comes with ABSOLUTELY NO WARRANTY.  This is free software, and you\n"
"are welcome to redistribute it under certain conditions.  You should have\n"
"received a copy of the GNU General Public License along with this program.  If\n"
"not, see <http://www.gnu.org/licenses/>.\n"
"\n"
"\n"
"Usage: cunf [OPTIONS] NETFILE\n"
"\n"
"Argument NETFILE is a path to the .ll_net input file.  Allowed OPTIONS are:\n"
" -t NAME      Stop when transition NAME is inserted\n"
" -d DEPTH     Unfold up to given DEPTH\n"
" -o FILE      Output file to store the unfolding in.  If not provided,\n"
"              defaults to NETFILE with the last 7 characters removed\n"
"              (extension '.ll_net') plus a suffix depending option the -O\n"
" -f FORMAT    Write unfolding in format FORMAT. Available formats: 'cuf',\n"
"              'dot', 'fancy'.  Default is 'cuf'.\n"
"\n"
"For more information, see http://www.lsv.ens-cachan.fr/Software/cunf/\n"
"Branch eager v1.6, compiled %s\n", __DATE__);

	exit (EXIT_FAILURE);
}

void rusage (void)
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
	std::vector<sat::Lit> c(2);

	p = s.new_var ();
	q = s.new_var ();
	r = s.new_var ();

	c[0] = ~p;
	c[1] = ~q;
	s.add_clause (c);

	auto ret = s.solve ();
	if (ret == sat::Cnf::SAT) {
		DEBUG ("SAT");
	} else if (ret == sat::Cnf::UNSAT) {
		DEBUG ("UNSAT");
	} else {
		DEBUG ("UNKNOWN");
	}
}


int main (int argc, char **argv)
{
	int ret;
	char *endptr;
	struct option longopts[] = {
			{"cutoff", required_argument, 0, 'c'},
			{"save", required_argument, 0, 's'},
			{0, 0, 0, 0}};

	/* initialize global parameters and options */
	u.mark = 2;
	u.stoptr = 0;
	opt.cutoffs = OPT_ERV;
	opt.save_path = 0;
	opt.depth = 0;

	test ();
	return 0;

	/* parse command line */
	while (1) {
		ret = getopt_long (argc, argv, "", longopts, 0);
		if (ret == -1) break;
		switch (ret) {
		case 'c' :
			if (strcmp (optarg, "mcmillan") == 0) {
				opt.cutoffs = OPT_MCMILLAN;
			} else if (strcmp (optarg, "erv") == 0) {
				opt.cutoffs = OPT_ERV;
			} else {
				opt.depth = strtol (optarg, &endptr, 10);
				if (optarg[0] == 0 || *endptr != 0) {
					usage ();
				}
				opt.cutoffs = OPT_DEPTH;
			}
			break;
		case 's' :
			opt.save_path = optarg;
			break;
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

#if 0
	while (1) {
		opt = getopt (argc, argv, "o:t:d:f:");
		if (opt == -1) break;
		switch (opt) {
		case 'o' :
			outpath = optarg;
			break;
		case 't' :
			stoptr = optarg;
			break;
		case 'd' :
			u.depth = atoi (optarg);
			break;
		case 'f' :
			outformat = optarg;
			break;
		default :
			usage ();
		}
	}
	if (optind != argc - 1) usage ();
	inpath = argv[argc - 1];

	/* validate the output format string */
	if (! outformat) outformat = (char *) "cuf";
	if (strcmp (outformat, "cuf") && strcmp (outformat, "dot") &&
			strcmp (outformat, "fancy")) usage ();

	/* set default file name for the output */
	if (! outpath) {
		l = strlen (inpath);
		outpath = (char *) gl_malloc (l + 16);
		strcpy (outpath, inpath);
		strcpy (outpath + (l > 7 ? l - 7 : l), ".unf.");
		strcpy (outpath + (l > 7 ? l - 2 : l + 5), 
				! strcmp (outformat, "fancy") ? "dot" :
				outformat);
	}
#endif

	/* load the input net */
	DPRINT ("  Reading net from '%s'\n", opt.net_path);
	read_pep_net (opt.net_path);
	DPRINT ("  It is a %s net\n", u.net.isplain ? "plain" : "contextual");
	nc_static_checks ();

	/* unfold */
	unfold ();

	/* write the output net */
	if (opt.save_path) {
		DPRINT ("  Writing unfolding to '%s'\n", opt.save_path);
		write_cuf (opt.save_path);
	}

#ifdef CONFIG_DEBUG
	db_mem ();
#endif
	rusage ();
	PRINT ("time\t%.3f\n"
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
