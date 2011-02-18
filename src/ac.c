
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

#include "global.h"
#include "al/al.h"
#include "glue.h"
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
		c = (struct cond *) e->pre.adj[i];

		/* and the postset of c */
		for (j = c->post.deg - 1; j >= 0; j--) {
			ep = (struct event *) c->post.adj[j];
			/* FIXME -- does it really hurts keeping arrows between e and e ? */
			if (ep == e) continue;
			/* 1. pre(e') intersects pre(e) */
			/* 5. pre(e') intersects pre(e) */
			al_add (&e->ac, ep);
			al_add (&ep->ac, e);
		}

		/* 3. post(e') intersects pre(e) */
		al_add (&c->pre->ac, e);

		/* we explore the context of c */
		for (j = c->cont.deg - 1; j >= 0; j--) {
			ep = (struct event *) c->cont.adj[j];
			/* FIXME -- does it really hurts keeping arrows between e and e ? */
			if (ep == e) continue;
			/* 6. context(e') intersects pre(e) */
			al_add (&ep->ac, e);
		}
	}

	/* we explore the context of e */
	for (i = e->cont.deg - 1; i >= 0; i--) {
		c = (struct cond *) e->cont.adj[i];

		/* and the postset of c */
		for (j = c->post.deg - 1; j >= 0; j--) {
			ep = (struct event *) c->post.adj[j];
			/* FIXME -- does it really hurts keeping arrows between e and e ? */
			if (ep == e) continue;
			/* 2. pre(e') intersects context(e) */
			al_add (&e->ac, ep);
		}

		/* 4. post(e') intersects context(e) */
		al_add (&c->pre->ac, e);
	}
}

int ac_test (struct event * e1, struct event * e2)
{
	return al_test (&e1->ac, e2);
}

