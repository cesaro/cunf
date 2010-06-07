
#include "marking.h"
#include "global.h"
#include "debug.h"
#include "glue.h"
#include "ac.h"
#include "dg.h"
#include "h.h"

static void _h_lists (struct h * h1, struct h * h2, struct dls * l1,
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

static int _h_t_cmp (void *n1, void *n2)
{
	return ((struct trans *) n1)->id - ((struct trans *) n2)->id;
}

static void _h_parikh_init (struct h *h, struct nl **l)
{
	/* initializes the list of transitions l and the number of involved
	 * transitions in this parikh vector */

	ASSERT (h->parikh.tab == 0);
	h->parikh.size = 0;
	*l = 0;
}

static void _h_parikh_add (struct h *h, struct nl **l, struct trans *t)
{
	/* increments the counter of occurrences of transition t in history h
	 * and increments the number of transitions if this is the first
	 * occurrence of t */

	/* if the count is zero, increment the number of transitions in the
	 * vector and append to the list l */
	if (t->parikhcnt1 == 0) {
		h->parikh.size++;
		nl_insert2 (l, t, _h_t_cmp);
	}
	t->parikhcnt1++;
}

static void _h_parikh_trans2vector (struct h *h, struct nl *l)
{
	struct trans *t;
	struct nl *n;
	int i;

	/* mallocates the parikh vector in h->parikh.tab, sets up the entries
	 * to the observed transitions and clears the count in each transition
	 */
	
	ASSERT (h->parikh.size >= 1);
	h->parikh.tab = gl_malloc (h->parikh.size * sizeof (struct parikh));

	i = 0;
	for (n = l; n; n = n->next) {
		t = (struct trans *) n->node;
		ASSERT (t->parikhcnt1 >= 1);
		h->parikh.tab[i].t = t;
		h->parikh.tab[i].count = t->parikhcnt1;
		i++;
		t->parikhcnt1 = 0; /* very important! */
	}

	ASSERT (i == h->parikh.size);
	nl_delete (l);

#ifdef CONFIG_DEBUG
	for (i = 0; i < h->parikh.size; i++) {
		if (i >= 1) ASSERT (h->parikh.tab[i - 1].t->id <
				h->parikh.tab[i].t->id);
		ASSERT (h->parikh.tab[i].count >= 1);
	}
#endif
}

void __h (struct dls * l)
{
	struct dls *n;
	struct h *h;

	for (n = l->next; n; n = n->next) {
		h = dls_i (struct h, n, auxnod);
		PRINT ("%02d h%d/e%d:%s\n", h->depth, h->id, h->e->id, h->e->origin->name);
	}
	PRINT ("\n");
}

static int _h_cmp_foata (struct h *h1, struct h *h2)
{
	int depth, found, found2;
	int m1, m2, m12;
	struct trans *t;
	struct dls l12;
	struct dls *n;
	struct dls l1;
	struct dls l2;
	struct nl *nn;
	struct nl *l;
	struct h *h;
	
	/* generate three different marks */
	m1 = ++u.mark;
	m2 = ++u.mark;
	m12 = ++u.mark;

	/* separate the events in h1 and h2 in three lists */
	_h_lists (h1, h2, &l1, &l2, &l12, m1, m2, m12);

	/* while both lists have events; we can safely ignore events in l12, as
	 * they  collaborate exactly in the same amount for foata vectors of
	 * every slice (depth) in both histories */
	depth = 1;
	while (l1.next && l2.next) {

		l = 0;
		/* push transitions at depth 'depth' to the list l, if they are
		 * not already there, and remove them from l1 */
		for (n = l1.next; n; n = n->next) {
			h = dls_i (struct h, n, auxnod);
			if (h->depth != depth) continue;
			dls_remove (&l1, n);
			t = h->e->origin;
			if (t->parikhcnt1 == 0 && t->parikhcnt2 == 0) {
				nl_insert2 (&l, t, _h_t_cmp);
			}
			t->parikhcnt1++;
		}

		/* same with list l2 */
		for (n = l2.next; n; n = n->next) {
			h = dls_i (struct h, n, auxnod);
			if (h->depth != depth) continue;
			dls_remove (&l2, n);
			t = h->e->origin;
			if (t->parikhcnt2 == 0 && t->parikhcnt1 == 0) {
				nl_insert2 (&l, t, _h_t_cmp);
			}
			t->parikhcnt2++;
		}

		/* parikh vectors of transitions at depth 'depth' in both
		 * histories are stored in fields parikhcnt1 and parikhcnt2 of
		 * transitions present in list l; search for a difference in
		 * both vectors and clear these two fields */
		found = 0;
		found2 = 0;
		for (nn = l; nn; nn = nn->next) {
			t = (struct trans *) nn->node;
			found = t->parikhcnt1 - t->parikhcnt2;
			t->parikhcnt1 = 0;
			t->parikhcnt2 = 0;
			if (found != 0) break;
		}
		if (nn != 0) nn = nn->next;
		for (; nn; nn = nn->next) {
			t = (struct trans *) nn->node;
			if (found2 == 0) {
				if (found < 0) {
					if (t->parikhcnt1 != 0) found2 = -found;
				} else {
					if (t->parikhcnt2 != 0) found2 = -found;
				}
			}
			t->parikhcnt1 = 0;
			t->parikhcnt2 = 0;
		}
		if (found2 == 0) found2 = found;
		nl_delete (l);
		if (found2 != 0) return found2;

		/* parikh vectors are the same for transitions present at this
		 * depth, go to the next depth */
		depth++;
	}

	/* if we still have events in one list, that history is greater than
	 * the other one */
	if (l1.next != 0) return 1;
	if (l2.next != 0) return -1;

	/* otherwise, both histories h1 and h2 are exactly equal */
	ASSERT (h1 == h2);
	return 0;
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

static int _h_confl_cnd_check (struct nl *conds, struct dls *l, int m)
{
	int i, j;
	struct h *h;
	struct nl *n;
	struct cond *c;
	struct event *e;

	/* check if at least one condition in conds is consumed by events in
	 * the list l, which we assume to be marked with m */

	/* if the list is empty, it cannot consume any condition */
	if (l->next == 0) return 0;

	ASSERT (dls_i (struct h, l->next, auxnod)->m == m);

	/* for each condition in the list */
	for (n = conds; n; n = n->next) {
		c = (struct cond *) n->node;

		/* check whether at least one event consuming c is present in l
		 */
		for (i = c->post.deg - 1; i >= 0; i--) {
			e = dg_i (struct event, c->post.adj[i], post);
			for (j = e->hist.deg - 1; j >= 0; j--) {
				h = dg_i (struct h, e->hist.adj[j], nod);
				if (h->m == m) return 1;
			}
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
	h->id = u.unf.numh++;
	h->e = e;
	dg_init (&h->nod);
	h->m = 0;
	h->marking = 0;
	h->parikh.tab = 0;
	h->depth = 0;

	/* set h->corr to something different to null, so that it looks like a
	 * cutoff; this avoids that h is used to create a new history till it
	 * gets out of the pe set */
	h->corr = h;

	/* size and parikh.size can still be garbage (see h_marking) */

	/* also, add this history to the histories associated to the event */
	dg_add (&e->hist, &h->nod);
	return h;
}

struct h * h_dup (struct h * h)
{
	struct h *nh;

	nh = gl_malloc (sizeof (struct h));
	nh->id = u.unf.numh++;
	nh->e = h->e;
	nh->m = 0;
	nh->marking = 0;
	nh->parikh.tab = 0;
	nh->depth = h->depth;

	/* see annotation in h_alloc */
	nh->corr = nh;

	/* size and parikh.size can still be garbage (see h_marking) */

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
	gl_free (h->parikh.tab);
	gl_free (h);
}

void h_add (struct h * h, struct h * hp)
{
	/* set up a dependency of (the event associated to) history h to (the
	 * event associated to) history hp, by means of an edge from node h to
	 * node hp */

	ASSERT (h->e != hp->e);
	ASSERT (hp->corr == 0); /* assert that hp is not a cutoff */
	dg_add (&h->nod, &hp->nod);
	if (hp->depth >= h->depth) h->depth = hp->depth + 1;
}

int h_conflict2 (struct h *h1, struct nl *cond1, struct h *h2,
		struct nl *cond2)
{
	struct dls l1, l2, l12;
	int m1, m2, m12;

	/* return the logical value of the next boolean expression:
	 * h1 or h2 is a cutoff, or
	 * h1 is in conflict with h2, or
	 * h2 is in conflict with h1, or
	 * h1 consumes at least one condition in conds2, or
	 * h2 consumes at least one condition in conds1
	 */

	/* display a conflict if one of the histories is a cutoff */
	if (h1->corr != 0 || h2->corr != 0) return 1;

	/* generate three different marks */
	m1 = ++u.mark;
	m2 = ++u.mark;
	m12 = ++u.mark;

	_h_lists (h1, h2, &l1, &l2, &l12, m1, m2, m12);

	/* h1 (h2) consumes condition c2 (c1) */
	if (_h_confl_cnd_check (cond2, &l1, m1)) return 1;
	if (_h_confl_cnd_check (cond1, &l2, m2)) return 1;

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
	struct nl *l, *pl;

	 /* 1. explore all events of history h and mark its preset
	  * 2. explore again all events of history h and append to the marking
	  * all places labeling conditions in the postset of each event
	  * 2'. profit the occasion of exploring the history again to build the
	  * parikh vector 
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
	 * postset of the original transition
	 * 2'. profit the occasion of exploring the history again to build the
	 * parikh vector  */
	l = 0;
	_h_parikh_init (h, &pl);
	for (n = auxl.next; n; n = n->next) {
		hp = dls_i (struct h, n, auxnod);
		for (i = hp->e->post.deg - 1; i >= 0; i--) {
			c = dg_i (struct cond, hp->e->post.adj[i], post);
			if (c->m != visited) nl_insert (&l, c->origin);
		}

		_h_parikh_add (h, &pl, hp->e->origin);
	}
	_h_parikh_trans2vector (h, pl);

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

	PRINT ("+ History h%d/e%d:%s; size %d; depth %d; marking ",
			h->id,
			h->e->id,
			h->e->origin->name,
			h->size,
			h->depth);
	marking_print (h);
	PRINT ("\n");
}

int h_isdup (struct h *h)
{
	/* history h is a duplicate if there exists another (different) history
	 * associated to event h->e with the same events as h */
	int i;
	struct h *hp;
	struct dls l1;
	struct dls l2;
	struct dls l12;
	int m1, m2, m12;

	ASSERT (h);
	ASSERT (h->e);

	/* generate three different marks */
	m1 = ++u.mark;
	m2 = ++u.mark;
	m12 = ++u.mark;

	/* check that every history pointed by h->e->hist is different to h */
	for (i = h->e->hist.deg - 1; i >= 0; i--) {
		hp = dg_i (struct h, h->e->hist.adj[i], nod);
		if (h == hp) continue;
		_h_lists (h, hp, &l1, &l2, &l12, m1, m2, m12); /* very expensive! */
		if (l1.next == l1.prev && l2.next == l2.prev) {
			if (dls_i (struct h, l1.next, auxnod) == h &&
					dls_i (struct h, l2.next, auxnod) == hp) {
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
	}

	return 0;

#if 0
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
#endif
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

static int _h_cmp (struct h *h1, struct h *h2)
{
	int i, min;

	/* check sizes */
	if (h1->size != h2->size) return h1->size - h2->size;

#ifdef CONFIG_MCMILLAN
	BREAK (1);
	return 0;
#endif

	/* sizes are equal, check parikh vectors and return the condition of h1
	 * to be lexicographically smaller to h2 */
	ASSERT (h1->parikh.size >= 1);
	ASSERT (h1->parikh.tab);
	ASSERT (h2->parikh.size >= 1);
	ASSERT (h2->parikh.tab);
	min = h1->parikh.size < h2->parikh.size ? h1->parikh.size :
			h2->parikh.size;
	for (i = 0; i < min; i++) {
		if (h1->parikh.tab[i].t != h2->parikh.tab[i].t) {
			return h1->parikh.tab[i].t->id -
					h2->parikh.tab[i].t->id;
		}
		if (h2->parikh.tab[i].count != h1->parikh.tab[i].count) {
			if (h2->parikh.tab[i].count > h1->parikh.tab[i].count) {
				return i + 1 < h1->parikh.size ? 1 : -1;
			} else {
				return i + 1 < h2->parikh.size ? -1 : 1;
			}
		}
	}
	if (i > min && h1->parikh.size != h2->parikh.size) {
		return h1->parikh.size - h2->parikh.size;
	}

	/* sizes and parikh vectors are equal, check foata stuff */
	return _h_cmp_foata (h1, h2);
}

int h_cmp (struct h *h1, struct h *h2)
{
	int ret;

	PRINT ("  cmp h%d/e%d:%s  h%d/e%d:%s",
			h1->id, h1->e->id, h1->e->origin->name,
			h2->id, h2->e->id, h2->e->origin->name);
	ret = _h_cmp (h1, h2);
	PRINT (" returns %d\n", ret);
	return ret;
}

