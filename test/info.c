
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

#include "glue.h"
#include "debug.h"
#include "global.h"
#include "netconv.h"
#include "readpep.h"
#include "unfold.h"

static void info (void)
{
	struct trans *t;
	struct place *p;
	struct ls *n;

	for (n = u.net.places.next; n; n = n->next) {
		p = ls_i (struct place, n, nod);
		printf ("places.pre %d places.cont %d places.post %d\n",
				p->pre.deg, p->cont.deg, p->post.deg);
	}
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);
		printf ("trans.pre %d trans.cont %d trans.post %d\n",
				t->pre.deg, t->cont.deg, t->post.deg);
	}
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
	info ();

	return EXIT_SUCCESS;
}

