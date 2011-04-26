
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

#include <stdlib.h>

#include "marking.h"
#include "dls/dls.h"
#include "global.h"
#include "al/al.h"
#include "da/da.h"
#include "debug.h"
#include "glue.h"
#include "pe.h"
#include "h.h"

struct da hda;
struct da tda;

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
			hp = (struct h *) h->nod.adj[i];
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
			hp = (struct h *) h->nod.adj[i];
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
			hp = (struct h *) h->nod.adj[i];
			if (hp->m == m12) continue;
			ASSERT (hp->m == m1);
			dls_remove (l1, &hp->auxnod);
			hp->m = m12;
			dls_append (l12, &hp->auxnod);
		}
	}

#ifdef CONFIG_DEBUG
	/* DPRINT ("H1 : "); db_h (h1);
	DPRINT ("H2 : "); db_h (h2);
	DPRINT ("l1 : "); */
	/* assert that all elements in l1 are marked with m1, that all ... */
	ASSERT (u.unf.e0->hist.deg == 1);
	ASSERT (((struct h *) u.unf.e0->hist.adj[0])->m == m12);
	for (n = l1->next; n; n = n->next) {
		h = dls_i (struct h, n, auxnod);
		ASSERT (h->m == m1);
		// DPRINT ("e%d:%s ", h->e->id, h->e->ft->name);
	}
	// DPRINT ("\nl2 : ");
	for (n = l2->next; n; n = n->next) {
		h = dls_i (struct h, n, auxnod);
		ASSERT (h->m == m2);
		// DPRINT ("e%d:%s ", h->e->id, h->e->ft->name);
	}
	// DPRINT ("\nl12 : ");
	for (n = l12->next; n; n = n->next) {
		h = dls_i (struct h, n, auxnod);
		ASSERT (h->m == m12);
		// DPRINT ("e%d:%s ", h->e->id, h->e->ft->name);
	}
	// DPRINT ("\n");
#endif
}

static int _h_t_cmp (const void *n1, const void *n2)
{
	return ((struct trans *) n1)->id - ((struct trans *) n2)->id;
}

static int _h_t_qsort_cmp (const void *p1, const void *p2)
{
	return (* (struct trans **) p1)->id - (* (struct trans **) p2)->id;
}

static void _h_parikh_init (struct h *h)
{
	ASSERT (h->parikh.tab == 0);
	h->parikh.size = 0;
}

static void _h_parikh_add (struct h *h, struct trans *t)
{
	if (t->parikhcnt1 == 0) {
		ASSERT (h->parikh.size <= tda.len);
		da_push (&tda, h->parikh.size, t, struct trans *);
	}
	t->parikhcnt1++;
}

static void _h_parikh_trans2vector (struct h *h)
{
	struct trans *t;
	int i;

	/* mallocates the parikh vector in h->parikh.tab, sets up the entries
	 * to the observed transitions and clears the count in each transition
	 */

	ASSERT (h->parikh.size >= 1);
	qsort (tda.tab, h->parikh.size, sizeof (struct trans *),
			_h_t_qsort_cmp);

	h->parikh.tab = gl_malloc (h->parikh.size * sizeof (struct parikh));
	for (i = 0; i < h->parikh.size; i++) {
		t = da_i (&tda, i, struct trans *);
		h->parikh.tab[i].t = t;
		h->parikh.tab[i].count = t->parikhcnt1;
		t->parikhcnt1 = 0; /* very important! */

		ASSERT (i < 1 || h->parikh.tab[i - 1].t->id <
				h->parikh.tab[i].t->id);
		ASSERT (h->parikh.tab[i].count >= 1);
	}
}

void __h (struct dls * l)
{
	struct dls *n;
	struct h *h;

	for (n = l->next; n; n = n->next) {
		h = dls_i (struct h, n, auxnod);
		DPRINT ("%02d h%d/e%d:%s\n", h->depth, h->id, h->e->id, h->e->ft->name);
	}
	DPRINT ("\n");
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

	/* while both lists have events */
	depth = 1;
	while (l1.next && l2.next) {

		l = 0;
		/* push transitions at depth 'depth' to the list l, if they are
		 * not already there, and remove them from l1 */
		for (n = l1.next; n; n = n->next) {
			h = dls_i (struct h, n, auxnod);
			if (h->depth != depth) continue;
			dls_remove (&l1, n);
			t = h->e->ft;
			if (t->parikhcnt1 == 0 && t->parikhcnt2 == 0) {
				nl_insert2 (&l, t, _h_t_cmp);
			}
			t->parikhcnt1++;
			DPRINT ("  foata depth %d l1 add t %s pkh1 %d pkh2 %d\n",
					depth, t->name, t->parikhcnt1,
					t->parikhcnt2);
		}

		/* same with list l2 */
		for (n = l2.next; n; n = n->next) {
			h = dls_i (struct h, n, auxnod);
			if (h->depth != depth) continue;
			dls_remove (&l2, n);
			t = h->e->ft;
			if (t->parikhcnt2 == 0 && t->parikhcnt1 == 0) {
				nl_insert2 (&l, t, _h_t_cmp);
			}
			t->parikhcnt2++;
			DPRINT ("  foata depth %d l2 add t %s pkh1 %d pkh2 %d\n",
					depth, t->name, t->parikhcnt1,
					t->parikhcnt2);
		}

		/* if l is empty, parikh vectors are the same even if we skip
		 * adding transitions from l12; if l is not empty, we need to
		 * take into account transitions from l12, so we push also
		 * transitions from l12 if l is not empty (it is also safe to
		 * unconditionally append transitions from l12) */
#ifdef CONFIG_ERV
		if (l) {
#else
		if (1) {
#endif
			for (n = l12.next; n; n = n->next) {
				h = dls_i (struct h, n, auxnod);
				if (h->depth > depth) continue;
				dls_remove (&l12, n);
				if (h->depth != depth) continue;
				t = h->e->ft;
				if (t->parikhcnt2 == 0 && t->parikhcnt1 == 0) {
					nl_insert2 (&l, t, _h_t_cmp);
				}
				t->parikhcnt1++;
				t->parikhcnt2++;
				DPRINT ("  foata depth %d l12 add t %s pkh1 %d pkh2 %d\n",
						depth, t->name, t->parikhcnt1,
						t->parikhcnt2);
			}
		}

		/* parikh vectors of transitions at depth 'depth' in both
		 * histories are stored in fields parikhcnt1 and parikhcnt2 of
		 * transitions present in list l; search for a difference in
		 * both vectors and clear these two fields */
		found = 0;
		found2 = 0;
#ifdef CONFIG_ERV_MOLE
		/* fix to mimic the unfold order used in mole (which is not
		 * exactly the ERV order) */
		for (nn = l; nn; nn = nn->next) {
			t = (struct trans *) nn->node;
			if (t->parikhcnt1) found2++;
			if (t->parikhcnt2) found2--;
		}
#endif
		for (nn = l; nn; nn = nn->next) {
			t = (struct trans *) nn->node;
			found = t->parikhcnt1 - t->parikhcnt2;
			DPRINT ("  foata depth %d t %s pkh1 %d pkh2 %d found %d\n", 
					depth, t->name, t->parikhcnt1,
					t->parikhcnt2, found);
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
			DPRINT ("  foata depth %d t %s pkh1 %d pkh2 %d found %d found2 %d\n", 
					depth, t->name, t->parikhcnt1,
					t->parikhcnt2, found, found2);
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

	/* FIXME -- otherwise, both histories h1 and h2 are exactly equal */
	// ASSERT (h1 == h2);
	return 0;
}

#if 0
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
			e = (struct event *) c->post.adj[i];
			for (j = e->hist.deg - 1; j >= 0; j--) {
				h = (struct h *) e->hist.adj[j];
				if (h->m == m) return 1;
			}
		}
	}
	return 0;
}
#endif

void h_list (struct dls *l, struct h *h)
{
	struct dls *n;
	register struct h *hp;
	register struct h *hpp;
	register int i;
	register int m;
 
	/* explore history h and and insert into list l */
	m = ++u.mark;
 	ASSERT (m > 0);
 
	dls_init (l);
	h->m = m;
	dls_append (l, &h->auxnod);
	for (n = l->next; n; n = n->next) {
		hp = dls_i (struct h, n, auxnod);
 		ASSERT (hp->m == m);
 		for (i = hp->nod.deg - 1; i >= 0; i--) {
 			hpp = (struct h *) hp->nod.adj[i];
			if (hpp->m == m) continue;
 			hpp->m = m;
			dls_append (l, &hpp->auxnod);
 		}
 	}
 }

void h_mark (struct h *h, register int m)
{
	int fst;
	register int i;
	register struct h *hp;
	register struct h *hpp;
	register int lst;

	/* explore history h and mark events with m */
	ASSERT (m > 0);

	fst = lst = 0;
	h->m = m;
	da_push (&hda, lst, h, struct h *);
	while (fst < lst) {
		hp = da_i (&hda, fst++, struct h *);
		ASSERT (hp->m == m);

		for (i = hp->nod.deg - 1; i >= 0; i--) {
			hpp = (struct h *) hp->nod.adj[i];
			if (hpp->m == m) continue;
			hpp->m = m;
			da_push (&hda, lst, hpp, struct h *);
		}
	}
}

void h_init (void)
{
	da_init (&hda, struct h *);
	da_init (&tda, struct trans *);
}

void h_term (void)
{
	da_term (&hda);
	da_term (&tda);
}

struct h * h_alloc (struct event * e)
{
	struct h * h;

	/* mallocate a new node in the history graph and set up a pointer to
	 * the associated event */
	h = gl_malloc (sizeof (struct h));
	h->id = u.unf.numh++;
	h->e = e;
	al_init (&h->nod);
	al_init (&h->ecl);
	h->m = 0;
	h->depth = 0;
	al_init (&h->rd);
	al_init (&h->sd);
	h->marking = 0;
	h->parikh.tab = 0;

	/* set h->corr to something different to null, so that it looks like a
	 * cutoff; this avoids that h is used to create a new history till it
	 * gets out of the pe set */
	h->corr = h;

	/* size and parikh.size can still be garbage (see h_marking) */

	/* also, add this history to the histories associated to the event */
	al_add (&e->hist, h);
	return h;
}

struct h * h_dup (struct h * h)
{
	struct h *nh;

	nh = gl_malloc (sizeof (struct h));
	nh->id = u.unf.numh++;
	nh->e = h->e;
	nh->m = 0;
	nh->depth = h->depth;
	nh->marking = 0;
	nh->parikh.tab = 0;

	/* see annotation in h_alloc */
	nh->corr = nh;

	/* size and parikh.size can still be garbage (see h_marking) */

	al_init (&nh->nod);
	al_cpy (&nh->nod, &h->nod);
	al_init (&nh->ecl);
	al_cpy (&nh->ecl, &h->ecl);
	al_init (&nh->rd);
	al_init (&nh->sd);

	al_add (&nh->e->hist, nh);
	return nh;
}

void h_free (struct h *h)
{
	al_rem (&h->e->hist, h);
	al_term (&h->nod);
	al_term (&h->ecl);
	al_term (&h->rd);
	al_term (&h->sd);
	nl_delete (h->marking);
	gl_free (h->parikh.tab);
	gl_free (h);
}

void h_add (struct h * h, struct h * hp)
{
	/* set up a dependency of (the event associated to) history h to (the
	 * event associated to) history hp, by means of an edge from node h to
	 * node hp */

	/* we are done if the edge is already present */
	ASSERT (h->e != hp->e);
	ASSERT (hp->corr == 0); /* assert that hp is not a cutoff */
	if (al_test (&h->nod, hp)) return;

	al_add (&h->nod, hp);
	if (hp->depth >= h->depth) h->depth = hp->depth + 1;
}

#if 0
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

#if 0
	static int i = 0;

	i++;
	if (h1->id < h2->id) {
		DPRINT ("  xxx %d %d\n", h1->id, h2->id);
	} else {
		DPRINT ("  xxx %d %d\n", h2->id, h1->id);
	}
#endif

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
#endif

#if 0
void h_marking_old (struct h *h)
{
	int i, j, s, visited;
	struct dls auxl, *n;
	struct h *hp, *hpp;
	struct nl *l;
	struct event *e;
	struct cond *c;

	/* 1. explore all events of history h and mark its preset
	 * 2. explore again all events of history h and append to the marking
	 * all places labeling conditions in the postset of each event (take
	 * care of the fact that *maybe* the postset of the event is still not
	 * built)
	 * 2'. profit the occasion of exploring the history again to build the
	 * parikh vector 
	 * 2''. if the place labeling a condition is appended to the marking,
	 * append to the adjacency list rd all events reading that condition
	 * that have been marked in 1.
	 * 3. append the postset of h->e->ft if needed
	 * 4. insert into sd those events in h reading from pre(h->e)
	 * 5. we are done! */

	visited = ++u.mark;
	ASSERT (visited > 0);

	/* 1. mark the preset of all events in the history */
	dls_init (&auxl);
	dls_append (&auxl, &h->auxnod);
	h->e->m = visited;

	s = 1;
	for (n = auxl.next; n; n = n->next) {
		hp = dls_i (struct h, n, auxnod);
		ASSERT (hp->e->m == visited);
		for (i = hp->nod.deg - 1; i >= 0; i--) {
			hpp = (struct h *) hp->nod.adj[i];
			if (hpp->e->m == visited) continue;
			hpp->e->m = visited;
			dls_append (&auxl, &hpp->auxnod);
			s++;
		}

		for (i = hp->e->pre.deg - 1; i >= 0; i--) {
			c = (struct cond *) hp->e->pre.adj[i];
			c->m = visited;
		}
	}

	/* 2. explore again all events and append places associated to the
	 * postset of the original transition
	 * 2'. profit the occasion of exploring the history again to build the
	 * parikh vector
	 * 2''. if the place labeling a condition is appended to the marking,
	 * append to the adjacency list rd all events reading that condition
	 * that have been marked in 1. */
	l = 0;
	_h_parikh_init (h);
	for (n = auxl.next; n; n = n->next) {
		hp = dls_i (struct h, n, auxnod);
		ASSERT (hp->e->post.deg == hp->e->ft->post.deg || hp == h);

		for (i = hp->e->post.deg - 1; i >= 0; i--) {
			c = (struct cond *) hp->e->post.adj[i];
			if (c->m == visited) continue;

			nl_insert (&l, c->fp);
			for (j = c->cont.deg - 1; j >= 0; j--) {
				e = (struct event *) c->cont.adj[j];
				if (e->m == visited) al_add (&h->rd, e);
			}
		}
		_h_parikh_add (h, hp->e->ft);
	}
	_h_parikh_trans2vector (h);

	/* 3. append the postset of h->e->ft if needed */
	if (h->e->post.deg == 0) {
		for (i = h->e->ft->post.deg - 1; i >= 0; i--) {
			nl_insert (&l, h->e->ft->post.adj[i]);
		}
	}

	/* 4. insert into sd those events in h reading from pre(h->e) */
	for (i = h->e->pre.deg - 1; i >= 0; i--) {
		c = (struct cond *) h->e->pre.adj[i];
		for (j = c->cont.deg - 1; j >= 0; j--) {
			e = (struct event *) c->cont.adj[j];
			if (e->m == visited) al_add (&h->sd, e);
		}
	}

	/* store the marking associated to the history h, and the number of
	 * events in the history in h->marking and h->size */
	ASSERT (s >= 1);
	ASSERT (s >= 2 || h->id == 0);
	h->marking = l;
	h->size = s;

	DPRINT ("+ History h%d/e%d:%s; size %d; depth %d; readers %d; "
			"ecs %d; marking ",
			h->id,
			h->e->id,
			h->e->ft->name,
			h->size,
			h->depth,
			h->rd.deg,
			h->ecl.deg);
#ifdef CONFIG_DEBUG
	marking_print (h);
#endif
	DPRINT ("\n");
}

void h_marking_new (struct h *h)
{
	int i, j, m, s;
	struct h *hp, *hpp;
	struct dls hl, cl;
	struct nl *l;
	struct event *e;
	struct cond *c;
	struct dls *n;

	m = ++u.mark;
	ASSERT (m > 0);

	dls_init (&cl);
	dls_init (&hl);
	h->e->m = m;
	dls_append (&hl, &h->auxnod);

	s = 1;
	_h_parikh_init (h);
	for (n = hl.next; n; n = n->next) {
		hp = dls_i (struct h, n, auxnod);
		ASSERT (hp->e->m == m);
;
		for (i = hp->nod.deg - 1; i >= 0; i--) {
			hpp = (struct h *) hp->nod.adj[i];
			if (hpp->e->m == m) continue;
			hpp->e->m = m;
			dls_append (&hl, &hpp->auxnod);
			s++;
		}

		for (i = hp->e->post.deg - 1; i >= 0; i--) {
			c = (struct cond *) hp->e->post.adj[i];
			if (c->m != m) dls_insert (&cl, &c->auxnod);
		}
		for (i = hp->e->pre.deg - 1; i >= 0; i--) {
			c = (struct cond *) hp->e->pre.adj[i];
			c->m = m;
		}
		_h_parikh_add (h, hp->e->ft);
	}
	_h_parikh_trans2vector (h);

	l = 0;
	for (n = cl.next; n; n = n->next) {
		c = dls_i (struct cond, n, auxnod);
		if (c->m == m) continue;
		nl_insert (&l, c->fp);

		for (i = c->cont.deg - 1; i >= 0; i--) {
			e = (struct event *) c->cont.adj[i];
			if (e->m == m) al_add (&h->rd, e);
		}
	}

	if (h->e->post.deg == 0) {
		for (i = h->e->ft->post.deg - 1; i >= 0; i--) {
			nl_insert (&l, h->e->ft->post.adj[i]);
		}
	}

	for (i = h->e->pre.deg - 1; i >= 0; i--) {
		c = (struct cond *) h->e->pre.adj[i];
		for (j = c->cont.deg - 1; j >= 0; j--) {
			e = (struct event *) c->cont.adj[j];
			if (e->m == m) al_add (&h->sd, e);
		}
	}

	ASSERT (s >= 1);
	ASSERT (s >= 2 || h->id == 0);
	h->size = s;
	h->marking = l;
}
#endif

void h_marking (struct h *h)
{
	int m2, s, fst, lst;
	struct h *hpp;
	struct nl *l;

	register int i;
	register int j;
	register int m;
	register struct cond *c;
	register struct event *e;
	register struct h * hp;

	m = ++u.mark;
	m2 = ++u.mark;
	ASSERT (m > 0);
	ASSERT (m2 > 0);

	fst = lst = 0;
	h->e->m = m;
	da_push (&hda, lst, h, struct h *);
	s = 1;
	_h_parikh_init (h);
	while (fst < lst) {
		hp = da_i (&hda, fst++, struct h *);
		ASSERT (hp->e->m == m);

		_h_parikh_add (h, hp->e->ft);
		for (i = hp->nod.deg - 1; i >= 0; i--) {
			hpp = (struct h *) hp->nod.adj[i];
			if (hpp->e->m == m) continue;
			hpp->e->m = m;
			da_push (&hda, lst, hpp, struct h *);
			s++;
		}

		for (i = hp->e->pre.deg - 1; i >= 0; i--) {
			((struct cond *) hp->e->pre.adj[i])->m = m;
		}
	}
	_h_parikh_trans2vector (h);

	l = 0;
	fst = 0;
	if (u.net.isplain) {
		while (fst < lst) {
			e = da_i (&hda, fst++, struct h *)->e;
			for (i = e->post.deg - 1; i >= 0; i--) {
				c = (struct cond *) e->post.adj[i];
				if (c->m != m) nl_insert (&l, c->fp);
			}
		}
	} else {
		while (fst < lst) {
			e = da_i (&hda, fst++, struct h *)->e;
			for (i = e->post.deg - 1; i >= 0; i--) {
				c = (struct cond *) e->post.adj[i];
				if (c->m == m) continue;
				c->m = m2;
				nl_insert (&l, c->fp);
			}
		}
		fst = 0;
		while (fst < lst) {
			e = da_i (&hda, fst++, struct h *)->e;
			for (i = e->cont.deg - 1; i >= 0; i--) {
				if (((struct cond *) e->cont.adj[i])->m == m2){
					al_add (&h->rd, e);
					break;
				}
			}
		}
	}

	if (h->e->post.deg == 0) {
		for (i = h->e->ft->post.deg - 1; i >= 0; i--) {
			nl_insert (&l, h->e->ft->post.adj[i]);
		}
	}

	if (! u.net.isplain) {
		for (i = h->e->pre.deg - 1; i >= 0; i--) {
			c = (struct cond *) h->e->pre.adj[i];
			for (j = c->cont.deg - 1; j >= 0; j--) {
				e = (struct event *) c->cont.adj[j];
				if (e->m == m) al_add (&h->sd, e);
			}
		}
	}

	ASSERT (s >= 1);
	ASSERT (s >= 2 || h->id == 0);
	h->size = s;
	h->marking = l;

	DPRINT ("+ History h%d/e%d:%s; size %d; depth %d; readers %d; "
			"ecs %d; marking ",
			h->id,
			h->e->id,
			h->e->ft->name,
			h->size,
			h->depth,
			h->rd.deg,
			h->ecl.deg);
#ifdef CONFIG_DEBUG
	marking_print (h);
#endif
	DPRINT ("\n");
}

#if 0
void h_marking (struct h *h)
{
	struct h h1;
	//struct h h2;
	struct nl *n;
	struct nl *nn;
	//struct nl *nnn;
	int i;

	h1 = *h;
	//h2 = *h;
	al_init (&h1.rd);
	al_init (&h1.sd);
	//al_init (&h2.rd);
	//al_init (&h2.sd);

	//h_marking_new (&h2);
	h_marking_old (&h1);
	h_marking_new2 (h);

	ASSERT (h1.size == h->size);
	//ASSERT (h2.size == h->size);
	ASSERT (al_cmp (&h1.rd, &h->rd) == 0);
	ASSERT (al_cmp (&h1.sd, &h->sd) == 0);
	//ASSERT (al_cmp (&h2.rd, &h->rd) == 0);
	//ASSERT (al_cmp (&h2.sd, &h->sd) == 0);

	//for (n = h->marking, nn = h1.marking, nnn = h2.marking; n; n = n->next, nn = nn->next, nnn = nnn->next) {
	for (n = h->marking, nn = h1.marking; n; n = n->next, nn = nn->next) {
		ASSERT (n->node == nn->node);
		//ASSERT (n->node == nnn->node);
	}

	ASSERT (h->parikh.size == h1.parikh.size);
	//ASSERT (h->parikh.size == h2.parikh.size);
	for (i = 0; i < h->parikh.size; i++) {
		ASSERT (h1.parikh.tab[i].t == h->parikh.tab[i].t);
		ASSERT (h1.parikh.tab[i].count == h->parikh.tab[i].count);
		//ASSERT (h2.parikh.tab[i].t == h->parikh.tab[i].t);
		//ASSERT (h2.parikh.tab[i].count == h->parikh.tab[i].count);
	}
	al_term (&h1.rd);
	al_term (&h1.sd);
	//al_term (&h2.rd);
	//al_term (&h2.sd);
	nl_delete (h1.marking);
	gl_free (h1.parikh.tab);
	//nl_delete (h2.marking);
	//gl_free (h2.parikh.tab);
}
#endif

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

	/* check that every history pointed by h->e->hist is different to h */
	for (i = h->e->hist.deg - 1; i >= 0; i--) {
		hp = (struct h *) h->e->hist.adj[i];
		if (h == hp) continue;

		/* generate three different marks */
		m1 = ++u.mark;
		m2 = ++u.mark;
		m12 = ++u.mark;

		_h_lists (h, hp, &l1, &l2, &l12, m1, m2, m12); /* very expensive! */
		if (l1.next == l1.prev && l2.next == l2.prev) {
			if (dls_i (struct h, l1.next, auxnod) == h &&
					dls_i (struct h, l2.next, auxnod) == hp) {
				DPRINT ("  History h%d/e%d:%s is a duplicate of "
						"h%d/e%d:%s\n",
						h->id,
						h->e->id,
						h->e->ft->name,
						hp->id,
						hp->e->id,
						hp->e->ft->name);
				u.unf.numduph++;
				ASSERT (0);
				return 1;
			}
		}
	}

	return 0;
}

int h_cmp (struct h *h1, struct h *h2)
{
	int i, min;

	/* check sizes */
	if (h1->size != h2->size) return h1->size - h2->size;

#ifdef CONFIG_MCMILLAN
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

#if 0
int h_cmp (struct h *h1, struct h *h2)
{
	int ret;

	DPRINT ("  cmp h%d/e%d:%s  h%d/e%d:%s",
			h1->id, h1->e->id, h1->e->ft->name,
			h2->id, h2->e->id, h2->e->ft->name);
	ret = _h_cmp (h1, h2);
	DPRINT (" returns %d\n", ret);
	return ret;
}
#endif

