
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

