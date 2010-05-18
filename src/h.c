
#include "marking.h"
#include "global.h"
#include "debug.h"
#include "glue.h"
#include "ac.h"
#include "dg.h"
#include "h.h"

static void _h_confl_lists (struct h * h1, struct h * h2, struct dls * l1,
		struct dls * l2, struct dls * l12, int m1, int m2, int m12)
{
	int i;
	struct h *h;
	struct h *hp;
	struct dls *n;

	/* explore history h1, mark with m1 and insert all nodes in the list l1
	 */
	h1->m = m1;
	dls_init (l1);
	dls_append (l1, &h1->auxnod);
	for (n = l1->next; n; n = n->next) {
		h = dls_i (struct h, n, auxnod);
		ASSERT (h->m == m1);
		for (i = h->nod.deg - 1; i >= 0; i--) {
			hp = dg_i (struct h, h->nod.adj[i], nod);
			if (hp->m == m1) continue;
			hp->m = m1;
			dls_append (l1, &hp->auxnod);
		}
	}

	/* explore history h2; if element marked with m1, extract from
	 * l1, mark with m12 and insert into l12; if element marked with m12 or
	 * m2, skip; otherwise, mark with m2 and insert into l2 */
	dls_init (l2);
	dls_init (l12);
	if (h2->m == m1) {
		dls_remove (l1, &h2->auxnod);
		h2->m = m12;
		dls_append (l12, &h2->auxnod);
	} else {
		h2->m = m2;
		dls_append (l2, &h2->auxnod);
	}
	for (n = l2->next; n; n = n->next) {
		h = dls_i (struct h, n, auxnod);
		ASSERT (h->m == m2);
		for (i = h->nod.deg - 1; i >= 0; i--) {
			hp = dg_i (struct h, h->nod.adj[i], nod);
			if (hp->m == m12) continue;
			if (hp->m == m2) continue;
			if (hp->m == m1) {
				dls_remove (l1, &hp->auxnod);
				hp->m = m12;
				dls_append (l12, &hp->auxnod);
			} else {
				dls_append (l2, &hp->auxnod);
				hp->m = m2;
			}
		}
	}

	/* children histories of all histories in l12 must also be inserted in
	 * l12 */
	BREAK (h1->id == 11 && h2->id == 7);
	for (n = l12->next; n; n = n->next) {
		h = dls_i (struct h, n, auxnod);
		ASSERT (h->m == m12);
		for (i = h->nod.deg - 1; i >= 0; i--) {
			hp = dg_i (struct h, h->nod.adj[i], nod);
			if (hp->m == m12) continue;
			ASSERT (hp->m == m1);
			dls_remove (l1, &hp->auxnod);
			hp->m = m12;
			dls_append (l12, &hp->auxnod);
		}
	}

	/* PRINT ("H1 : "); db_h (h1);
	PRINT ("H2 : "); db_h (h2);
	PRINT ("l1 : "); */
	/* assert that all elements in l1 are marked with m1, that all ... */
	ASSERT (u.unf.e0->hist.deg == 1);
	ASSERT (dg_i (struct h, u.unf.e0->hist.adj[0], nod)->m == m12);
	for (n = l1->next; n; n = n->next) {
		h = dls_i (struct h, n, auxnod);
		ASSERT (h->m == m1);
		// PRINT ("e%d:%s ", h->e->id, h->e->origin->name);
	}
	// PRINT ("\nl2 : ");
	for (n = l2->next; n; n = n->next) {
		h = dls_i (struct h, n, auxnod);
		ASSERT (h->m == m2);
		// PRINT ("e%d:%s ", h->e->id, h->e->origin->name);
	}
	// PRINT ("\nl12 : ");
	for (n = l12->next; n; n = n->next) {
		h = dls_i (struct h, n, auxnod);
		ASSERT (h->m == m12);
		// PRINT ("e%d:%s ", h->e->id, h->e->origin->name);
	}
	// PRINT ("\n");
}

static int _h_confl_check (struct dls * l1, struct dls * l2, struct dls * l12)
{
	struct dls * n, * np;
	struct h * h, * hp;

	/* check that, for all events h->e in l2, there is no event hp->e in
	 * either l1 or l12 such that asym-confl (h->e, hp->e) */

	for (n = l2->next; n; n = n->next) {
		h = dls_i (struct h, n, auxnod);
		for (np = l1->next; np; np = np->next) {
			hp = dls_i (struct h, np, auxnod);
			if (ac_test (h->e, hp->e)) return 1;
		}
		for (np = l12->next; np; np = np->next) {
			hp = dls_i (struct h, np, auxnod);
			if (ac_test (h->e, hp->e)) return 1;
		}
	}
	return 0;
}

static int _h_confl_cnd_check (struct cond *c, struct dls *l, int m)
{
	int i, j;
	struct h *h;
	struct event *e;

	/* check if condition c is consumed by events in the list l, which we
	 * assume to be marked with m */

	/* if the list is empty, it cannot consume c */
	if (l->next == 0) return 0;

	ASSERT (dls_i (struct h, l->next, auxnod)->m == m);

	/* otherwise, we check whether at least one event consuming c is
	 * present in l */
	for (i = c->post.deg - 1; i >= 0; i--) {
		e = dg_i (struct event, c->post.adj[i], post);
		for (j = e->hist.deg - 1; j >= 0; j--) {
			h = dg_i (struct h, e->hist.adj[j], nod);
			if (h->m == m) return 1;
		}
	}
	return 0;
}

struct h * h_alloc (struct event * e)
{
	struct h * h;

	/* mallocate a new node in the history graph and set up a pointer to
	 * the associated event */
	h = gl_malloc (sizeof (struct h));
	h->e = e;
	h->m = 0;
	h->marking = 0;	
	h->size = 0;	
	h->id = u.unf.numh++;
	dg_init (&h->nod);

	/* also, add this history to the histories associated to the event */
	dg_add (&e->hist, &h->nod);
	return h;
}

struct h * h_dup (struct h * h)
{
	struct h *nh;

	nh = gl_malloc (sizeof (struct h));

	nh->e = h->e;
	nh->marking = 0;	
	nh->size = 0;	
	nh->id = u.unf.numh++;

	dg_init (&nh->nod);
	dg_cpy (&nh->nod, &h->nod);
	dg_add (&nh->e->hist, &nh->nod);
	return nh;
}

void h_free (struct h *h)
{
	dg_rem (&h->e->hist, &h->nod);
	dg_term (&h->nod);
	nl_delete (h->marking);
	gl_free (h);
}

void h_add (struct h * h, struct h * hp)
{
	/* set up a dependency of (event associated to) history h to (event
	 * associated to) history hp, by means of an edge from node h to node
	 * hp */

	ASSERT (h->e != hp->e);
	dg_add (&h->nod, &hp->nod);
}

int h_conflict (struct h *h1, struct h *h2)
{
	struct dls l1, l2, l12;
	int m1, m2, m12;

	/* return the logical condition of history h1 being in conflict to h2
	 */

	/* generate three different marks */
	m1 = ++u.mark;
	m2 = ++u.mark;
	m12 = ++u.mark;

	/* build l1, the list of events in only in h1, l2, the list of evets
	 * only in h2 and l12, the list of events in both h1 and h2 */
	_h_confl_lists (h1, h2, &l1, &l2, &l12, m1, m2, m12);

	if (_h_confl_check (&l1, &l2, &l12)) return 1;
	return _h_confl_check (&l2, &l1, &l12);
}

int h_conflict2 (struct h *h1, struct cond *c1, struct h *h2, struct cond *c2)
{
	struct dls l1, l2, l12;
	int m1, m2, m12;

	/* return the logical value of the next boolean expression:
	 * h1 is in conflict with h2, or
	 * h2 is in conflict with h1, or
	 * h1 consumes condition c2, or
	 * h2 consumes condition c1
	 */

	/* generate three different marks */
	m1 = ++u.mark;
	m2 = ++u.mark;
	m12 = ++u.mark;

	_h_confl_lists (h1, h2, &l1, &l2, &l12, m1, m2, m12);

	/* h1 (h2) consumes condition c2 (c1) */
	if (_h_confl_cnd_check (c2, &l1, m1)) return 1;
	if (_h_confl_cnd_check (c1, &l2, m2)) return 1;

	/* h1 (h2) is in conflict with h2 (h1) */
	if (_h_confl_check (&l1, &l2, &l12)) return 1;
	return _h_confl_check (&l2, &l1, &l12);
}

void h_marking (struct h *h)
{
	int i, s, visited;
	struct dls auxl, *n;
	struct h *hp, *hpp;
	struct place *p;
	struct cond *c;
	struct nl * l;

	 /* 1. explore all events of history h and mark it preset
	  * 2. explore again all events of history h and append to the marking
	  * all places labeling conditions in the postset of each event
	  * 3. note that, at this point, postset of event h->e may still not
	  * built, so append to the marking the postset of h->e->origin if
	  * needed :)
	  * 4. we are done!  */

	visited = ++u.mark;
	ASSERT (visited > 0);

	/* 1. mark the preset of all events in the history */
	dls_init (&auxl);
	dls_append (&auxl, &h->auxnod);
	h->m = visited;

	s = 1;
	for (n = auxl.next; n; n = n->next) {
		hp = dls_i (struct h, n, auxnod);
		ASSERT (hp->m == visited);
		for (i = hp->nod.deg - 1; i >= 0; i--) {
			hpp = dg_i (struct h, hp->nod.adj[i], nod);
			if (hpp->m == visited) continue;
			hpp->m = visited;
			dls_append (&auxl, &hpp->auxnod);
			s++;
		}

		ASSERT (hp->e);
		for (i = hp->e->pre.deg - 1; i >= 0; i--) {
			c = dg_i (struct cond, hp->e->pre.adj[i], pre);
			c->m = visited;
		}
	}

	/* 2. explore again all events and append places associated to the
	 * postset of the original transition */
	l = 0;
	for (n = auxl.next; n; n = n->next) {
		hp = dls_i (struct h, n, auxnod);
		for (i = hp->e->post.deg - 1; i >= 0; i--) {
			c = dg_i (struct cond, hp->e->post.adj[i], post);
			if (c->m != visited) nl_insert (&l, c->origin);
		}
	}

	/* 3. append the postset of h->e->origin if needed */
	if (h->e->post.deg == 0) {
		for (i = h->e->origin->post.deg - 1; i >= 0; i--) {
			p = dg_i (struct place, h->e->origin->post.adj[i],
					post);
			nl_insert (&l, p);
		}
	}

	/* store the marking associated to the history h, and the number of
	 * events in the history in h->marking and h->size */
	ASSERT (s >= 1);
	if (h->id != 0) ASSERT (s >= 2);
	h->marking = l;
	h->size = s;

	PRINT ("+ History h%d/e%d:%s; size %d; marking ",
			h->id,
			h->e->id,
			h->e->origin->name,
			h->size);
	marking_print (h);
	PRINT ("\n");
}

int h_isdup (const struct h *h)
{
	int i;

	/* history h is a duplicate if there exists another (different) history
	 * associated to event h->e isomorphic to it (same set of outgoing
	 * edges) */

	ASSERT (h);
	ASSERT (h->e);

	for (i = h->e->hist.deg - 1; i >= 0; i--) {
		if (&h->nod == h->e->hist.adj[i]) continue;
		if (dg_cmp (&h->nod, h->e->hist.adj[i]) == 0) {
			struct h *hp = dg_i (struct h, h->e->hist.adj[i], nod);
			PRINT ("  History h%d/e%d:%s is a duplicate of "
					"h%d/e%d:%s\n",
					h->id,
					h->e->id,
					h->e->origin->name,
					hp->id,
					hp->e->id,
					hp->e->origin->name);
			return 1;
		}
	}
	return 0;
}

void h_list (struct dls *l, struct h *h)
{
	struct dls *n;
	struct h *hp, *hpp;
	int m, i;

	/* explore history h, mark events with m and insert in the list l */

	m = ++u.mark;
	h->m = m;
	dls_init (l);
	dls_append (l, &h->auxnod);
	for (n = l->next; n; n = n->next) {
		hp = dls_i (struct h, n, auxnod);
		ASSERT (hp->m == m);
		for (i = hp->nod.deg - 1; i >= 0; i--) {
			hpp = dg_i (struct h, hp->nod.adj[i], nod);
			if (hpp->m == m) continue;
			hpp->m = m;
			dls_append (l, &hpp->auxnod);
		}
	}
}

