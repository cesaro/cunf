
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "util/system.h"
#include "util/misc.h"
#include "cunf/global.h"
#include "cunf/netconv.h"
#include "cunf/readpep.h"
#include "cunf/unfold.h"

#define P printf

struct u u;
struct opt opt;

static void write_net_dot (void)
{
	struct trans *t;
	struct place *p;
	struct ls *n;
	int i;

	/* mark the places in the initial marking */
	for (i = u.net.t0->post.deg - 1; i >= 0; i--) {
		p = (struct place *) u.net.t0->post.adj[i];
		p->m = 1;
	}

	P ("digraph {\n\t/* places */\n");
	P ("\tnode    [style=filled fillcolor=gray90 shape=circle];\n");
	for (n = u.net.places.next; n; n = n->next) {
		p = ls_i (struct place, n, nod);
		P ("\tp%-6d [label=\"%s\"];%s\n", p->id, p->name,
				p->m ? " /* initial */" : "");
	}

	P ("\n\t/* transitions */\n");
	P ("\tnode    [shape=box style=filled fillcolor=grey60];\n");
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);
		if (t->id == 0) { ASSERT (strcmp (t->name, "_t0_") == 0); }
		if (t->id == 0) continue;
		P ("\tt%-6d [label=\"%s\"];\n", t->id, t->name);
	}

	P ("\n\t/* postset of each transition */\n");
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);
		if (t->id == 0) continue;

		for (i = t->post.deg - 1; i >= 0; i--) {
			p = (struct place *) t->post.adj[i];
			P ("\tt%-6d -> p%d;\n", t->id, p->id);
		}
	}

	P ("\n\t/* preset and context of each transition */\n");
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);
		if (t->id == 0) continue;
		for (i = t->pre.deg - 1; i >= 0; i--) {
			p = (struct place *) t->pre.adj[i];
			P ("\tp%-6d -> t%d;\n", p->id, t->id);
		}

		for (i = t->cont.deg - 1; i >= 0; i--) {
			p = (struct place *) t->cont.adj[i];
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
		ut_err ("Invalid arguments, see the code!");
	}

	read_pep_net (argv[1]);
	nc_static_checks ();
	// db_net ();
	write_net_dot ();

	return EXIT_SUCCESS;
}

