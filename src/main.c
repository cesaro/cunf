
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "glue.h"
#include "debug.h"
#include "global.h"
#include "netconv.h"
#include "readpep.h"
#include "unfold.h"

/* all gloabal data is stored in this structure */
struct u u;

/*****************************************************************************/

void usage (const char *myname)
{
	fprintf(stderr,
		"%s -- low level net unfolder\n\n"
		"Usage: %s [options] <LLnetfile> [FileOptions]\n\n"

	"     Options:\n"
	"      -T <name>      stop when transition <name> is inserted\n"
	"      -d <depth>     unfold up to given depth\n"
	"      -i             interactive mode\n\n"

	"     FileOptions:\n"
	"      -m <filename>  file to store the unfolding in\n\n"

	"Unless specified otherwise, all filenames will default to\n"
	"the basename of <LLnetfile> plus appropriate extensions.\n\n"

	"Version 2, " __DATE__ "\n", myname, myname);

	exit (EXIT_SUCCESS);
}

void write_dot (const char * filename)
{
	int i, m, enr, hnr;
	struct dls l, *ln;
	struct h *h, *hp;
	struct event *e;
	struct cond *c;
	struct ls *n;
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
			hnr);
#ifdef CONFIG_MCMILLAN
	P ("\tUsing McMillan order<br align=\"left\"/>\n");
#else
	P ("\tUsing Esparza/Romer/Vogler order<br align=\"left\"/>\n");
#endif
	P ("\t>];\n");
	P ("}\n");

	i = fclose (f);
	if (i == EOF) gl_err ("'%s': %s", filename, strerror (errno));

	PRINT ("  %d events, %d conditions, %d histories. Have a nice day!\n",
			u.unf.numev - 1, u.unf.numco, hnr);
}

int main (int argc, char **argv)
{
	int	 i;
	char    *llnet = NULL, *dotfile;
	char    **dptr = &llnet;
	char	*tmpname, *idx, *sptr;

	/* initialize global parameters */
	u.stoptr = 0;
	u.depth = 0;
	u.interactive = 0;

	u.mark = 1;

	/* parse command line */
	sptr = 0;
	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i],"-m"))
			dptr = &dotfile;
		else if (!strcmp(argv[i],"-T"))
			sptr = argv[++i];
		else if (!strcmp(argv[i],"-d"))
			u.depth = atoi(argv[++i]);
		else if (!strcmp(argv[i],"-i"))
			u.interactive = 1;
		else
		{
			if (!dptr) usage(argv[0]);
			*dptr = argv[i];

			tmpname = gl_malloc (strlen(argv[i]) + 16);
			strcpy(tmpname,argv[i]);
			idx = strrchr(tmpname,'.');
			if (!idx) strcat(tmpname,".");
			idx = strrchr(tmpname,'.');

			if (dptr == &llnet)
			{
				strcpy(idx,".unf.dot");
				dotfile = gl_strdup (tmpname);
			}

			dptr = NULL;
		}

	if (!llnet) usage (argv[0]);

	DPRINT ("  Reading net from '%s'\n", llnet);
	read_pep_net (llnet);
	nc_static_checks (sptr);
	unfold ();
	PRINT ("  Done, writing unfolding to '%s'\n", dotfile);
	write_dot (dotfile);
	return EXIT_SUCCESS;
}

