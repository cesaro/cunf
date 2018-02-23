
/* 
 * Copyright (C) 2014 Cesar Rodriguez <cesar.rodriguez@lipn.fr>
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

#define P printf

struct u u;
struct opt opt;

static void write_net_pt1 (void)
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

	P ("PT1\n%d\n%d\n", u.net.numpl, u.net.numtr);
	i = 0;
	for (n = u.net.places.next; n; n = n->next) {
		p = ls_i (struct place, n, nod);
		p->id = i;
		i++;
		P ("\"%s\" %d\n", p->name, p->m ? 1 : 0);
	}

	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);
		if (t->id == 0) {
			ASSERT (strcmp (t->name, "_t0_") == 0);
			continue;
		}
		if (t->cont.deg != 0) ut_err ("The net has read arcs, aborting!");
		P ("\"%s\" %d %d", t->name, t->pre.deg, t->post.deg);
		for (i = t->pre.deg - 1; i >= 0; i--) {
			p = (struct place *) t->pre.adj[i];
			P (" %d", p->id);
		}
		for (i = t->post.deg - 1; i >= 0; i--) {
			p = (struct place *) t->post.adj[i];
			P (" %d", p->id);
		}
		P ("\n");
	}
}

int main (int argc, char ** argv)
{
	if (argc != 2 || argv[1] == 0) {
		ut_err ("Invalid arguments, see the code!");
	}

	read_pep_net (argv[1]);
	nc_static_checks ();
	// db_net ();
	write_net_pt1 ();

	return EXIT_SUCCESS;
}

