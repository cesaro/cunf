
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

#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "glue.h"
#include "debug.h"
#include "global.h"
#include "netconv.h"
#include "readpep.h"
#include "unfold.h"

/* all gloabal data is stored in this structure */
struct u u;

/*****************************************************************************/

void usage (void)
{
	fprintf(stderr,
"\n"
"The cunf tool -- a low level contextual Petri net unfolder\n"
"\n"
"Copyright (C) 2011  Cesar Rodriguez <cesar.rodriguez@lsv.ens-cachan.fr>\n"
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
" -T NAME      Stop when transition NAME is inserted\n"
" -d DEPTH     Unfold up to given DEPTH\n"
" -m FILE      Output file to store the unfolding in.  If not provided,\n"
"              defaults to NETFILE with the last 7 characters removed\n"
"              (extension '.ll_net') plus the suffix '.unf.dot'\n"
"\n"
"For more information, see http://www.lsv.ens-cachan.fr/Software/cunf/\n"
"Branch eager r37, compiled %s\n", __DATE__);

	exit (EXIT_FAILURE);
}

void write_dot (const char * filename)
{
	struct event *e;
	struct cond *c;
	struct ls *n;
	int i, m;
	FILE *f;

#define P(args...) fprintf (f, ##args)

	f = fopen (filename, "w");
	if (f == 0) {
		gl_err ("'%s': %s", filename, strerror (errno));
		return;
	}

	m = ++u.mark;
	ASSERT (m > 0);
	for (i = u.unf.e0->post.deg - 1; i >= 0; i--) {
		c = (struct cond *) u.unf.e0->post.adj[i];
		c->m = m;
	}

	P ("digraph x {\nnode [shape=box];\n");
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		P ("e%d [label=\"%s:e%d\"];\n", e->id, e->ft->name, e->id);
		/* printf ("event.pre %d event.cont %d event.post %d\n",
				e->pre.deg, e->cont.deg, e->post.deg); */
	}

	P ("node [shape=circle];\n");
	for (n = u.unf.conds.next; n; n = n->next) {
		c = ls_i (struct cond, n, nod);
		P (c->m == m ? "c%d [label=\"%s:c%d\"]; /* initial */\n" :
				"c%d [label=\"%s:c%d\"];\n",
				c->id, c->fp->name, c->id);
		/* printf ("cond.pre 1 cond.cont %d cond.post %d\n",
				c->cont.deg, c->post.deg);
		struct ls * nn;
		for (nn = c->ecl.next; nn; nn = nn->next) {
			struct ec * r = ls_i (struct ec, nn, nod);
			printf ("ec.co %d ec.gen %d ec.read %d ec.comp %d\n", r->co.deg, EC_ISGEN (r), EC_ISREAD (r), EC_ISCOMP (r));
		} */
	}

	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		for (i = e->post.deg - 1; i >= 0; i--) {
			c = (struct cond *) e->post.adj[i];
			P ("e%d -> c%d;\n", e->id, c->id);
		}
	}

	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		for (i = e->pre.deg - 1; i >= 0; i--) {
			c = (struct cond *) e->pre.adj[i];
			P ("c%d -> e%d;\n", c->id, e->id);
		}
		for (i = e->cont.deg - 1; i >= 0; i--) {
			c = (struct cond *) e->cont.adj[i];
			P ("c%d -> e%d [arrowhead=none];\n", c->id, e->id);
		}
	}

	P ("}\n");
	i = fclose (f);
	if (i == EOF) gl_err ("'%s': %s", filename, strerror (errno));
}

void write_dot_fancy (const char * filename)
{
	int i, m, enr, hnr;
	struct dls l, *ln;
	struct h *h, *hp;
	struct event *e;
	struct cond *c;
	struct ls *n;
	FILE *f;

	f = fopen (filename, "w");
	if (f == 0) {
		gl_err ("'%s': %s", filename, strerror (errno));
		return;
	}

	m = ++u.mark;
	ASSERT (m > 0);
	for (i = u.unf.e0->post.deg - 1; i >= 0; i--) {
		c = (struct cond *) u.unf.e0->post.adj[i];
		c->m = m;
	}

	P ("digraph unfolding {\n\t/* events */\n");
	P ("\tnode    [shape=box style=filled fillcolor=grey60];\n");
	enr = 0;
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		enr++;
		P ("\te%-6d [label=\"%s:e%d\"%s];\n",
				e->id,
				e->ft->name,
				e->id,
				e->iscutoff ? " shape=Msquare" : "");
	}

	P ("\n\t/* conditions */\n");
	P ("\tnode    [style=filled fillcolor=gray90 shape=circle];\n");
	for (n = u.unf.conds.next; n; n = n->next) {
		c = ls_i (struct cond, n, nod);

		P ("\tc%-6d [label=\"%s:c%d\"];%s\n",
				c->id, c->fp->name, c->id,
				c->m == m ? " /* initial */" : "");
	}

	P ("\n\t/* history annotations for events */\n");
	P ("\tedge    [color=white];\n");
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		P ("\te%-6d -> e%d [label=\"", e->id, e->id);
		for (i = e->hist.deg - 1; i >= 0; i--) {
			h = (struct h *) e->hist.adj[i];
			P ("h%d%s", h->id, i == 0 ? "" : ",");
		}
		P ("\"];\n");
	}

	P ("\n\t/* postset of events */\n");
	P ("\tedge    [color=black];\n");
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;

		for (i = e->post.deg - 1; i >= 0; i--) {
			c = (struct cond *) e->post.adj[i];
			P ("\te%-6d -> c%d;\n", e->id, c->id);
		}
	}

	P ("\n\t/* preset and context of events */\n");
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;

		for (i = e->pre.deg - 1; i >= 0; i--) {
			c = (struct cond *) e->pre.adj[i];
			P ("\tc%-6d -> e%d;\n", c->id, e->id);
		}

		for (i = e->cont.deg - 1; i >= 0; i--) {
			c = (struct cond *) e->cont.adj[i];
			P ("\tc%-6d -> e%d [arrowhead=none color=red];\n",
					c->id, e->id);
		}
	}

	P ("\n\t/* histories */\n");
	P ("\tgraph [fontname=\"Courier\" label=< <br/>\n");
	P ("\tHistory Depth Size Events<br align=\"left\"/>\n");
	hnr = 0;
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		for (i = e->hist.deg - 1; i >= 0; i--) {
			h = (struct h *) e->hist.adj[i];
			hnr++;
			P ("\t%sh%-5d %5d %4d ",
					h->corr ? "*" : " ", h->id,
					h->depth, h->size);
			h_list (&l, h);
			for (ln = l.next; ln; ln = ln->next) {
				hp = dls_i (struct h, ln, auxnod);
				P ("e%d:%s%s ",
						hp->e->id,
						hp->e->ft->name,
						ln->next ? "," : "");
			}
			P ("<br align=\"left\"/>\n");
		}
	}

	ASSERT (enr == u.unf.numev - 1);
	ASSERT (hnr == u.unf.numh - u.unf.numduph - 1);
	P ("\t <br align=\"left\"/>\n");
	P ("\t%d transitions<br align=\"left\"/>\n"
			"\t%d places<br align=\"left\"/>\n"
			"\t%d events<br align=\"left\"/>\n"
			"\t%d conditions<br align=\"left\"/>\n"
			"\t%d histories<br align=\"left\"/>\n",
			u.net.numtr,
			u.net.numpl,
			u.unf.numev - 1,
			u.unf.numco,
			u.unf.numh - u.unf.numduph - 1);
#ifdef CONFIG_MCMILLAN
	P ("\tUsing McMillan order<br align=\"left\"/>\n");
#else
	P ("\tUsing Esparza/Romer/Vogler order<br align=\"left\"/>\n");
#endif
	P ("\t>];\n");
	P ("}\n");

	i = fclose (f);
	if (i == EOF) gl_err ("'%s': %s", filename, strerror (errno));
}

void rusage (void)
{
	struct rusage r;
	char buff[128];
	int ret;

	u.unf.usrtime = 0;
	u.unf.vmsize = 0;
	ret = getrusage (RUSAGE_SELF, &r);
	if (ret >= 0) {
		/* in linux this is 0; in mac os this is the maxrss in bytes */
		u.unf.vmsize = r.ru_maxrss / 1024;
		u.unf.usrtime = r.ru_utime.tv_sec * 1000 +
				r.ru_utime.tv_usec / 1000;
	}

	/* this will only work in linux, in macos vmsize is set to maxrss */
	ret = open ("/proc/self/statm", O_RDONLY);
	if (ret < 0) return;
	read (ret, buff, 128);
	close (ret);
	buff[127] = 0;
	u.unf.vmsize = strtoul (buff, 0, 10) * sysconf (_SC_PAGESIZE) >> 10;
}

char * peakmem (void)
{
	static char b[16];
	char buff[4096];
	char *s;
	int fd, ret;

	fd = open ("/proc/self/status", O_RDONLY);
	if (fd < 0) return "?";

	ret = read (fd, buff, 4096);
	close (fd);
	if (ret >= 4096) {
		PRINT ("Bug in peakmem!!\n");
		exit (1);
	}
	buff[ret] = 0;
	s = strstr (buff, "VmPeak:\t");
	if (! s) return "?";
	s[16] = 0;
	sprintf (b, "%u", atoi (s + 8));
	return b;
}

int main (int argc, char **argv)
{
	int opt, l;
	char *stoptr, *inpath, *outpath;

	/* initialize global parameters */
	u.mark = 1;

	/* parse command line */
	stoptr = 0;
	inpath = 0;
	outpath = 0;
	u.stoptr = 0;
	u.depth = 0;
	while (1) {
		opt = getopt (argc, argv, "m:T:d:");
		if (opt == -1) break;
		switch (opt) {
		case 'm' :
			outpath = optarg;
			break;
		case 'T' :
			stoptr = optarg;
			break;
		case 'd' :
			u.depth = atoi (optarg);
			break;
		default :
			usage ();
		}
	}
	if (optind != argc - 1) usage ();
	inpath = argv[argc - 1];

	/* set default file name for the output */
	if (! outpath) {
		l = strlen (inpath);
		outpath = gl_malloc (l + 16);
		strcpy (outpath, inpath);
		strcpy (outpath + (l > 7 ? l - 7 : l), ".unf.dot");
	}

	/* load the input net */
	DPRINT ("  Reading net from '%s'\n", inpath);
	read_pep_net (inpath);
	DPRINT ("  It is a %s net\n", u.net.isplain ? "plain" : "contextual");
	nc_static_checks (stoptr);

	/* unfold */
	unfold ();

	/* write the output net */
	DPRINT ("  Writing unfolding to '%s'\n", outpath);
#ifdef CONFIG_DEBUG
	write_dot (outpath);
	db_mem ();
#else
	write_dot (outpath);
#endif

	/* int s = 1000 * 1000 * 500;
	char * p = malloc (s);
	for (s--; s; s--) p[s] = 'c'; */
	rusage ();
	PRINT ("time\tmem\thist\tevents\tcond\tnocut\tgen\tread\tcomp\trd\tsd\tnet\n");
	PRINT ("%.2f\t%ld\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\n",
		u.unf.usrtime / 1000.0,
		u.unf.vmsize / 1024,
		u.unf.numh - u.unf.numduph - 1,
		u.unf.numev - 1,
		u.unf.numco,
		u.unf.numh - u.unf.numduph - 1 - u.unf.numcutoffs,
		u.unf.numgenecs,
		u.unf.numreadecs,
		u.unf.numcompecs,
		u.unf.numrd,
		u.unf.numsd,
		inpath);
	return EXIT_SUCCESS;
}

