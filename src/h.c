
#include "marking.h"
#include "global.h"
#include "debug.h"
#include "glue.h"
#include "ac.h"
#include "dg.h"
#include "h.h"

static void _h_confl_lists (struct h * h1, struct h * h2, struct dls * l1,
		struct dls * l2, struct dls * l12)
{
	int i, m1, m2, m12;
	struct h *h;
	struct h *hp;
	struct dls *n;

	/* generate three different marks */
	m1 = ++u.mark;
	m2 = ++u.mark;
	m12 = ++u.mark;

	/* explore history h1, mark with m1 insert all nodes in the dls list
	 * l1 */
	h1->m = m1;
	dls_init (l1);
	dls_insert (l1, &h1->auxnod);
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
		dls_insert (l12, &h2->auxnod);
	} else {
		h2->m = m2;
		dls_insert (l2, &h2->auxnod);
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
				dls_insert (l12, &hp->auxnod);
			} else {
				dls_append (l2, &hp->auxnod);
				hp->m = m2;
			}
		}
	}

	/* children histories of all histories in l12 must also be inserted in
	 * l12 */
	for (n = l12->next; n; n = n->next) {
		h = dls_i (struct h, n, auxnod);
		ASSERT (h->m == m12);
		for (i = h->nod.deg - 1; i >= 0; i--) {
			hp = dg_i (struct h, h->nod.adj[i], nod);
			if (hp->m == m12) continue;
			ASSERT (hp->m == m1);
			dls_remove (l1, &hp->auxnod);
			hp->m = m12;
			dls_insert (l12, &hp->auxnod);
		}
	}

	/* assert that all elements in l1 are marked with m1, that all ... */
	for (n = l1->next; n; n = n->next) {
		ASSERT (dls_i (struct h, n, auxnod)->m == m1);
	}
	for (n = l2->next; n; n = n->next) {
		ASSERT (dls_i (struct h, n, auxnod)->m == m2);
	}
	for (n = l12->next; n; n = n->next) {
		ASSERT (dls_i (struct h, n, auxnod)->m == m12);
	}
}

static int _h_confl_check (struct dls * l1, struct dls * l2, struct dls * l12)
{
	struct dls * n, * np;
	struct h * h, * hp;

	/* check that, for all histories h in l2, there is no history hp in
	 * either l1 or l12 such that asym-confl (h, hp) */

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

void h_add (struct h * h, struct h * hp)
{
	/* set up a dependency of (event associated to) history h to (event
	 * associated to) history hp, by means of an edge from node h to node
	 * hp */

	ASSERT (h->e != hp->e);
	dg_add (&h->nod, &hp->nod);
}

int h_conflict (struct h * h1, struct h * h2)
{
	struct dls l1, l2, l12;

	/* return the logical condition of history h1 being in conflict to h2
	 */

	_h_confl_lists (h1, h2, &l1, &l2, &l12);

	if (_h_confl_check (&l1, &l2, &l12)) return 1;
	return _h_confl_check (&l2, &l1, &l12);
}

void h_marking (struct h *h)
{
	struct dg *pre, *post;
	int i, s, visited, ispre;
	struct dls auxl, *n;
	struct h *hp, *hpp;
	struct place *p;
	struct nl * l;

	/* to compute the marking associated to h we explore all events in the
	 * history h and mark the preset of the transition associated to each
	 * event in the history; next, we explore again the list of events and
	 * grab all places not previously marked in the postset of the
	 * transition associated to each event; not very efficient :( */

	visited = ++u.mark;
	ASSERT (visited > 0);

	dls_init (&auxl);
	dls_insert (&auxl, &h->auxnod);
	h->m = visited;

	s = 1;
	for (n = auxl.next; n; n = n->next) {
		hp = dls_i (struct h, n, auxnod);
		ASSERT (h->m == visited);
		for (i = hp->nod.deg - 1; i >= 0; i--) {
			hpp = dg_i (struct h, hp->nod.adj[i], nod);
			if (hpp->m == visited) continue;
			hpp->m = visited;
			dls_append (&auxl, &hpp->auxnod);
			s++;
		}

		ASSERT (hp->e);
		ASSERT (hp->e->origin);
		/* FIXME -- we skip this to be able to run this function and
		 * not modify transition marks :/  Less performance but still
		 * correct */
		/* if (hp->e->origin->m == visited) continue;
		hp->e->origin->m = visited; */
		pre = &hp->e->origin->pre;
		for (i = pre->deg - 1; i >= 0; i--) {
			p = dg_i (struct place, pre->adj[i], pre);
			p->m = visited;
		}
	}

	/* important, renew the visited mark !! */
	ispre = visited;
	visited = ++u.mark;
	ASSERT (visited > 0);

	l = 0;
	for (n = auxl.next; n; n = n->next) {
		hp = dls_i (struct h, n, auxnod);
		/* FIXME -- we skip this to be able to run this function and
		 * not modify transition marks :/  Less performance but still
		 * correct */
		/* if (hp->e->origin->m == visited) continue;
		hp->e->origin->m = visited; */

		post = &hp->e->origin->post;
		for (i = post->deg - 1; i >= 0; i--) {
			p = dg_i (struct place, post->adj[i], post);
			if (p->m != ispre) nl_insert (&l, p);
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

