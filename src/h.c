
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
				if (c->m != m) {
					nl_insert (&l, c->fp);
					u.unf.nummrk++;
				}
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
				u.unf.nummrk++;
			}
		}
		fst = 0;
		while (fst < lst) {
			e = da_i (&hda, fst++, struct h *)->e;
			for (i = e->cont.deg - 1; i >= 0; i--) {
				c = (struct cond *) e->cont.adj[i];
				/* optimz: e not interesting if fp (c) has
 				 * empty postset */
				//if (c->m == m2 && c->fp->post.deg != 0) {
				if (c->m == m2) {
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
		u.unf.nummrk += h->e->ft->post.deg;
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
	h->hash = marking_hash (l);

	u.unf.numr += h->rd.deg;
	u.unf.nums += h->sd.deg;

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

