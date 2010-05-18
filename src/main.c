
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#define P printf

void write_dot (void)
{
	struct dls l, *ln;
	struct h *h, *hp;
	struct event *e;
	struct cond *c;
	struct ls *n;
	int i, enr;

	P ("digraph {\n\t/* events */\n");
	P ("\tnode    [shape=box style=filled fillcolor=grey60];\n");
	enr = 0;
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		// if (e->id == 0) continue;
		enr++;
		P ("\te%-6d [label=\"e%d:%s\"%s];\n",
				e->id,
				e->id,
				e->origin->name,
				e->iscutoff ? " shape=Msquare" : "");
	}

	P ("\n\t/* conditions */\n");
	P ("\tnode    [style=filled fillcolor=gray90 shape=circle];\n");
	for (n = u.unf.conds.next; n; n = n->next) {
		c = ls_i (struct cond, n, nod);

		P ("\tc%-6d [label=\"c%d:%s\"];\n",
				c->id, c->id, c->origin->name);
	}

	P ("\n\t/* history annotations for events */\n");
	P ("\tedge    [color=white];\n");
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		// if (e->id == 0) continue;
		P ("\te%-6d -> e%d [label=\"", e->id, e->id);
		for (i = e->hist.deg - 1; i >= 0; i--) {
			h = dg_i (struct h, e->hist.adj[i], nod);
			P ("h%d%s", h->id, i == 0 ? "" : ",");
		}
		P ("\"];\n");
	}

	P ("\n\t/* postset of events */\n");
	P ("\tedge    [color=black];\n");
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		// if (e->id == 0) continue;

		for (i = e->post.deg - 1; i >= 0; i--) {
			c = dg_i (struct cond, e->post.adj[i], post);
			P ("\te%-6d -> c%d;\n", e->id, c->id);
		}
	}

	P ("\n\t/* preset and context of events */\n");
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		// if (e->id == 0) continue;

		for (i = e->pre.deg - 1; i >= 0; i--) {
			c = dg_i (struct cond, e->pre.adj[i], pre);
			P ("\tc%-6d -> e%d;\n", c->id, e->id);
		}

		for (i = e->cont.deg - 1; i >= 0; i--) {
			c = dg_i (struct cond, e->cont.adj[i], cont);
			P ("\tc%-6d -> e%d [arrowhead=none color=red];\n", c->id, e->id);
		}
	}

	P ("\n\t/* histories */\n");
	P ("\tgraph [fontname=\"Courier\" label=< <br/>\n");
	P ("\tHistory Size Events<br align=\"left\"/>\n");
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		for (i = e->hist.deg - 1; i >= 0; i--) {
			h = dg_i (struct h, e->hist.adj[i], nod);
			P ("\th%-6d %4d ", h->id, h->size);
			h_list (&l, h);
			for (ln = l.next; ln; ln = ln->next) {
				hp = dls_i (struct h, ln, auxnod);
				P ("e%d:%s%s ",
						hp->e->id,
						hp->e->origin->name,
						ln->next ? "," : "");
			}
			P ("<br align=\"left\"/>\n");
		}
	}

	ASSERT (enr == u.unf.numev);
	P ("\t <br align=\"left\"/>\n");
	P ("\t%d transitions<br align=\"left\"/>\n"
			"\t%d places<br align=\"left\"/>\n"
			"\t%d events<br align=\"left\"/>\n"
			"\t%d conditions<br align=\"left\"/>\n"
			"\t%d histories<br align=\"left\"/>\n",
			u.net.numtr,
			u.net.numpl,
			enr,
			u.unf.numco,
			u.unf.numh);
	P ("\t>];\n");
	P ("}\n");
}

int main (int argc, char **argv)
{
	int	 i;
	char    *llnet = NULL, *mcifile;
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
			dptr = &mcifile;
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

			tmpname = gl_malloc (strlen(argv[i])+5);
			strcpy(tmpname,argv[i]);
			idx = strrchr(tmpname,'.');
			if (!idx) strcat(tmpname,".");
			idx = strrchr(tmpname,'.');

			if (dptr == &llnet)
			{
				strcpy(idx,".mci");
				mcifile = gl_strdup (tmpname);
			}

			dptr = NULL;
		}

	if (!llnet) usage (argv[0]);

	read_pep_net (llnet);
	nc_static_checks (sptr);
	db_net ();
	unfold ();
	write_dot ();

	return EXIT_SUCCESS;
}

