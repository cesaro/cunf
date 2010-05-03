
#include "global.h"
#include "debug.h"
#include "dg.h"
#include "ac.h"

void ac_add (struct event * e)
{
	struct cond * c;
	struct event * ep;
	int i, j;

	/* The directed graph built using the field "ac" in the event structre
	 * keeps record of the asymmetric conflict relation.  There is an edge
	 * e1 -> e2 in the graph for each pair of events e1 and e2 such that e1
	 * is in asymmetric conflict to e2 (e1 must fire before e2).  This
	 * function is called each time we add an event e to the unfolding.  We
	 * compute here and append to the graph edges to/from all the events e'
	 * such that e -> e' or e' -> e.
	 *
	 * In particular, we estabilish an edge from event e to any event e'
	 * such that:
	 * 1. pre(e') intersects pre(e)
	 * 2. pre(e') intersects context(e)
	 *
	 * Also, we set an edge from any event e' to event e if:
	 * 3. post(e') intersects pre(e)
	 * 4. post(e') intersects context(e)
	 * 5. pre(e') intersects pre(e)
	 * 6. context(e') intersects pre(e)
	 */

	/* we explore the preset of e */
	for (i = e->pre.deg - 1; i >= 0; i--) {
		c = dg_i (struct cond, e->pre.adj[i], pre);

		/* and the postset of c */
		for (j = c->post.deg - 1; j >= 0; j--) {
			ep = dg_i (struct event, c->post.adj[j], post);
			/* FIXME -- does it really hurts keeping arrows between e and e ? */
			if (ep == e) continue;
			/* 1. pre(e') intersects pre(e) */
			/* 5. pre(e') intersects pre(e) */
			dg_add (&e->ac, &ep->ac);
			dg_add (&ep->ac, &e->ac);
		}

		/* 3. post(e') intersects pre(e) */
		dg_add (&c->pre->ac, &e->ac);

		/* we explore the context of c */
		for (j = c->cont.deg - 1; j >= 0; j--) {
			ep = dg_i (struct event, c->cont.adj[j], cont);
			/* FIXME -- does it really hurts keeping arrows between e and e ? */
			if (ep == e) continue;
			/* 6. context(e') intersects pre(e) */
			dg_add (&ep->ac, &e->ac);
		}
	}

	/* we explore the context of e */
	for (i = e->cont.deg - 1; i >= 0; i--) {
		c = dg_i (struct cond, e->cont.adj[i], cont);

		/* and the postset of c */
		for (j = c->post.deg - 1; j >= 0; j--) {
			ep = dg_i (struct event, c->post.adj[j], post);
			/* FIXME -- does it really hurts keeping arrows between e and e ? */
			if (ep == e) continue;
			/* 2. pre(e') intersects context(e) */
			dg_add (&e->ac, &ep->ac);
		}

		/* 4. post(e') intersects context(e) */
		dg_add (&c->pre->ac, &e->ac);
	}
}

int ac_test (struct event * e1, struct event * e2)
{
	return dg_test (&e1->ac, &e2->ac);
}

