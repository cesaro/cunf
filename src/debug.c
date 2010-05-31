
#include "config.h"
#include "global.h"
#include "debug.h"
#include "glue.h"
#include "h.h"

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

void db_h (struct h *h)
{
	static int m = 1;
	struct dls l, *n;
	struct h *hp, *hpp;
	int i, s;

	/* generate a new mark */
	m++;

	dls_init (&l);
	dls_append (&l, &h->debugnod);
	h->debugm = m;

	s = 1;
	for (n = l.next; n; n = n->next) {
		hp = dls_i (struct h, n, debugnod);
		ASSERT (hp->debugm == m);
		for (i = hp->nod.deg - 1; i >= 0; i--) {
			hpp = dg_i (struct h, hp->nod.adj[i], nod);
			if (hpp->debugm == m) continue;
			hpp->debugm = m;
			dls_append (&l, &hpp->debugnod);
			s++;
		}
	}
	/* ASSERT (s == h->size); we might call this function after h is
	 * completely initialized */

/*
 h12/e34:T123; depth 5; size 7; e123:T12, e3:T12, e0:__t0
*/
	PRINT ("h%d/e%d:%s; depth %d; size %d; ",
			h->id, h->e->id, h->e->origin->name, h->depth, s);
	for (n = l.next; n; n = n->next) {
		hp = dls_i (struct h, n, debugnod);
		PRINT ("e%d:%s, ", hp->e->id, hp->e->origin->name);
	}
	PRINT ("\n");
}

void db_hgraph (void)
{
	struct event *e;
	struct h *h;
	struct ls *n;
	int i;

	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		for (i = e->hist.deg - 1; i >= 0; i--) {
			h = dg_i (struct h, e->hist.adj[i], nod);
			db_h (h);
		}
	}
	PRINT ("\n");
}

void db_c (struct cond *c)
{
	struct event *e;
	int i;

	PRINT ("c%d:%s  pre e%d:%s;  post ",
			c->id,
			c->origin->name,
			c->pre->id,
			c->pre->origin->name);

	for (i = c->post.deg - 1; i >= 0; i--) {
		e = dg_i (struct event, c->post.adj[i], post);
		PRINT ("e%d:%s ", e->id, e->origin->name);
	}

	PRINT ("\b;  cont ");
	for (i = c->cont.deg - 1; i >= 0; i--) {
		e = dg_i (struct event, c->cont.adj[i], cont);
		PRINT ("e%d:%s ", e->id, e->origin->name);
	}
	PRINT ("\b;\n");
}

void db_e (struct event *e)
{
	struct cond *c;
	int i;

	PRINT ("e%d:%s  pre ",
			e->id,
			e->origin->name);

	for (i = e->pre.deg - 1; i >= 0; i--) {
		c = dg_i (struct cond, e->pre.adj[i], pre);
		PRINT ("c%d:%s ", c->id, c->origin->name);
	}
	PRINT ("\b;  post ");

	for (i = e->post.deg - 1; i >= 0; i--) {
		c = dg_i (struct cond, e->post.adj[i], post);
		PRINT ("c%d:%s ", c->id, c->origin->name);
	}

	PRINT ("\b;  cont ");
	for (i = e->cont.deg - 1; i >= 0; i--) {
		c = dg_i (struct cond, e->cont.adj[i], cont);
		PRINT ("c%d:%s ", c->id, c->origin->name);
	}
	PRINT ("\b;\n");
}

