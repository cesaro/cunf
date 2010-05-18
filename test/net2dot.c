
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "glue.h"
#include "debug.h"
#include "global.h"
#include "netconv.h"
#include "readpep.h"
#include "unfold.h"

#define P printf

static void write_net_dot (void)
{
	struct trans *t;
	struct place *p;
	struct ls *n;
	int i;

	P ("digraph {\n\t/* places */\n");
	P ("\tnode    [style=filled fillcolor=gray90 shape=circle];\n");
	for (n = u.net.places.next; n; n = n->next) {
		p = ls_i (struct place, n, nod);
		P ("\tp%-6d [label=\"%s\"];\n", p->id, p->name);
	}

	P ("\n\t/* transitions */\n");
	P ("\tnode    [shape=box style=filled fillcolor=grey60];\n");
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);
		P ("\tt%-6d [label=\"%s\"];\n", t->id, t->name);
	}

	P ("\n\t/* postset of each transition */\n");
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);

		for (i = t->post.deg - 1; i >= 0; i--) {
			p = dg_i (struct place, t->post.adj[i], post);
			P ("\tt%-6d -> p%d;\n", t->id, p->id);
		}
	}

	P ("\n\t/* preset and context of each transition */\n");
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);
		for (i = t->pre.deg - 1; i >= 0; i--) {
			p = dg_i (struct place, t->pre.adj[i], pre);
			P ("\tp%-6d -> t%d;\n", p->id, t->id);
		}

		for (i = t->cont.deg - 1; i >= 0; i--) {
			p = dg_i (struct place, t->cont.adj[i], cont);
			P ("\tp%-6d -> t%d [arrowhead=none color=red];\n",
					p->id, t->id);
		}
	}

	P ("\n");
	P ("\tgraph   [label=\"%d transitions\\n%d places\"];\n",
			u.net.numtr,
			u.net.numpl);
	P ("}\n");
}

int main (int argc, char ** argv)
{
	ASSERT (argc == 2);
	ASSERT (argv[1] != 0);

	if (argc != 2 || argv[1] == 0) {
		gl_err ("Invalid arguments, see the code!");
	}

	read_pep_net (argv[1]);
	nc_static_checks (0);
	// db_net ();
	write_net_dot ();

	return EXIT_SUCCESS;
}

