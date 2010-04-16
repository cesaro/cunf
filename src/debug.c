
#include "config.h"
#include "global.h"
#include "glue.h"
#include "debug.h"

void breakme (void)
{
}

void db_net (void)
{
	struct place * p;
	struct trans * t;
	struct ls * n;
	struct nl * nl;

	printf ("Net, %d places, %d transitions; maxpre %d, maxpost "
			"%d, maxcont %d\n",
			u.net.numpl,
			u.net.numtr,
			u.net.maxpre,
			u.net.maxpost,
			u.net.maxcont);

	for (n = u.net.places.next; n; n = n->next) {
		p = ls_item (struct place, n, nod);
		printf ("   Place, num %d name '%s' %s\n"
				"      pre: { ",
				p->num,
				p->name,
				p->marked ? "marked" : "");
		for (nl = p->pre; nl; nl = nl->next) {
			printf ("%d ", ((struct trans *) nl->node)->num);
		}
		printf ("}\n      post: { ");
		for (nl = p->post; nl; nl = nl->next) {
			printf ("%d ", ((struct trans *) nl->node)->num);
		}
		printf ("}\n      cont: { ");
		for (nl = p->cont; nl; nl = nl->next) {
			printf ("%d ", ((struct trans *) nl->node)->num);
		}
		printf ("}\n");
	}

	printf ("\n");
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_item (struct trans, n, nod);
		printf ("   Trans, num %d name '%s'\n"
				"      pre: size %d { ",
				t->num,
				t->name,
				t->pre_size);
		for (nl = t->pre; nl; nl = nl->next) {
			printf ("%d ", ((struct place *) nl->node)->num);
		}
		printf ("}\n      post: size %d { ", t->post_size);
		for (nl = t->post; nl; nl = nl->next) {
			printf ("%d ", ((struct place *) nl->node)->num);
		}
		printf ("}\n      cont: size %d { ", t->cont_size);
		for (nl = t->cont; nl; nl = nl->next) {
			printf ("%d ", ((struct place *) nl->node)->num);
		}
		printf ("}\n");
	}
}

