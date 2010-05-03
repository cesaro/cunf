
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
	int i;

	PRINT ("Net, %d places, %d transitions; maxpre %d, maxpost "
			"%d, maxcont %d\n",
			u.net.numpl,
			u.net.numtr,
			u.net.maxpre,
			u.net.maxpost,
			u.net.maxcont);

	for (n = u.net.places.next; n; n = n->next) {
		p = ls_i (struct place, n, nod);
		PRINT ("   Place, id %d name '%s' %s\n"
				"      pre: { ",
				p->id,
				p->name,
				p->m ? "marked" : "");
		for (i = 0; i < p->pre.deg; i++) {
			t = dg_i (struct trans, p->pre.adj[i], pre);
			PRINT ("%d ", t->id);
		}
		PRINT ("}\n      post: { ");
		for (i = 0; i < p->post.deg; i++) {
			t = dg_i (struct trans, p->post.adj[i], post);
			PRINT ("%d ", t->id);
		}
		PRINT ("}\n      cont: { ");
		for (i = 0; i < p->cont.deg; i++) {
			t = dg_i (struct trans, p->cont.adj[i], cont);
			PRINT ("%d ", t->id);
		}
		PRINT ("}\n");
	}

	PRINT ("\n");
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);
		PRINT ("   Trans, id %d name '%s'\n"
				"      pre: size %d { ",
				t->id,
				t->name,
				t->pre.deg);
		for (i = 0; i < t->pre.deg; i++) {
			p = dg_i (struct place, t->pre.adj[i], pre);
			PRINT ("%d ", p->id);
		}
		PRINT ("}\n      post: size %d { ", t->post.deg);
		for (i = 0; i < t->post.deg; i++) {
			p = dg_i (struct place, t->post.adj[i], post);
			PRINT ("%d ", p->id);
		}
		PRINT ("}\n      cont: size %d { ", t->cont.deg);
		for (i = 0; i < t->cont.deg; i++) {
			p = dg_i (struct place, t->cont.adj[i], cont);
			PRINT ("%d ", p->id);
		}
		PRINT ("}\n");
	}
}

