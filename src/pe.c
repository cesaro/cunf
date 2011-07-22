
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

#include "string.h"
#include "global.h"
#include "al/al.h"
#include "ls/ls.h"
#include "debug.h"
#include "glue.h"
#include "ec.h"
#include "h.h"

#include "pe.h"

struct comb_entry {
	int size;
	int nr;
	int i;
	struct ec **tab;
	int ispre;
};

struct comb {
	struct comb_entry *tab;
	int size;

	struct ec *r;
	int ispre;
	struct trans *t;
};

struct q {
	int size;
	struct h ** tab;
	int skip;
};

struct pe {
	struct q q;
	struct comb comb;
};

struct pe pe;

/* invariants for the priority queue q:
 * 1. The queue contains exactly size items, from position 1 to position
 *    size, and size+1 slots in the tab array. Position 0 is empty.
 * 2. size is 0 when the queue is empty
 * 3. After calling _pe_alloc, position tab[size] is available for writing
 * 4. For any index with 1 <= index <= size, if index*2+1 is <= size, then
 *    tab[index] <= tab[index*2] and tab[index] <= tab[index*2+1]
 */

static void _pe_q_alloc (void)
{
	/* if current size is a power of two */
	if ((pe.q.size & (pe.q.size - 1)) == 0) {

		/* 2 slots is the minimum size */
		if (pe.q.size == 0) {
			pe.q.tab = (struct h **) gl_realloc (0,
					2 * sizeof (struct h *));
			return;
		}

		/* duplicate the size */
		pe.q.tab = (struct h **) gl_realloc (pe.q.tab,
				pe.q.size * 2 * sizeof (struct h *));
	}
}

static void _pe_q_insert (struct h * h)
{
	int idx;

	/* if we reached the maximum depth (-d option) or we already found the
	 * stop transition (-T option), skip the insertion */
	if (u.depth && h->depth > u.depth) return;
	if (pe.q.skip) return;
	
	/* make room for a new item and proceed with the insertion */
	pe.q.size++;
	idx = pe.q.size;
	_pe_q_alloc ();

	/* when the -T is used and u.stoptr is found, empty the queue to make
	 * sure that the corresponding event is processed immediately. Also,
	 * prevent any further additions to the PE queue. */
	if (h->e->ft == u.stoptr) {
		pe.q.skip = 1;
		pe.q.size = 1;
		pe.q.tab[1] = h;
		return;
	}

	/* insert the new element at the end, then move upwards as needed */
	for (; idx > 1; idx /= 2) {
		if (h_cmp (h, pe.q.tab[idx / 2]) > 0) break;
		pe.q.tab[idx] = pe.q.tab[idx / 2]; /* move parent downwards */
	}
	pe.q.tab[idx] = h;
}

static struct event * _pe_comb_instance_of (void)
{
	int m, i, idx, more;
	struct trans *t;
	struct event *e;
	struct cond *c;
	struct ls *n;

	/* given transition pe.comb.t and conditions pe.comb.r and pe.comb.tab,
	 * determine whether there is already an occurrence of t firing from
	 * those conditions */

	/* optimz: if pe.comb.r->c is in preset of such event, and its preset
	 * is empty, indeed no such event exist! */
	if (pe.comb.ispre) {
		if (pe.comb.r->c->post.deg == 0) return 0;
	} else {
		if (pe.comb.r->c->cont.deg == 0) return 0;
	}

	/* optimz: if every condition in the comb has currently only one
	 * history, then for sure t has no occurrences */
	ASSERT (pe.comb.r->c->ecl.next == &pe.comb.r->nod);
	more = pe.comb.r->nod.next != 0;

	m = ++u.mark;
	ASSERT (m > 0);
	pe.comb.r->c->m = m;
	for (i = 0; i < pe.comb.size; i++) {
		idx = pe.comb.tab[i].i;
		pe.comb.tab[i].tab[idx]->c->m = m;

		/* optimz: if it is not the first or not the last, then there
		 * is more than one enriched condition */
		if (pe.comb.tab[i].tab[idx]->c->ecl.next !=
			&pe.comb.tab[i].tab[idx]->nod ||
			pe.comb.tab[i].tab[idx]->nod.next != 0) more++;
	}

	/* optimz: if only one h. per cond., then there is no occurrence of t */
	if (more == 0) return 0;
	t = pe.comb.t;
	ASSERT (pe.comb.size + 1 == t->pre.deg + t->cont.deg);

	for (n = t->events.next; n; n = n->next) {
		e = ls_i (struct event, n, tnod);
		for (i = e->pre.deg - 1; i >= 0; i--) {
			c = (struct cond *) e->pre.adj[i];
			if (c->m != m) break;
		}
		if (i >= 0) continue;
		for (i = e->cont.deg - 1; i >= 0; i--) {
			c = (struct cond *) e->cont.adj[i];
			if (c->m != m) break;
		}
		if (i < 0) return e;
	}
	return 0;
}

static struct event * _pe_comb_new_event (void)
{
	struct trans *t;
	struct event *e;
	struct cond *c;
	int i;

	t = pe.comb.t;
	ASSERT (t);
	ASSERT (t->pre.deg + t->cont.deg == pe.comb.size + 1);

	/* allocate and initialize the event */
	e = gl_malloc (sizeof (struct event));
	ls_insert (&u.unf.events, &e->nod);
	al_init (&e->pre);
	al_init (&e->post);
	al_init (&e->cont);
	al_init (&e->ac);
	al_init (&e->hist);
	e->ft = t;
	ls_insert (&t->events, &e->tnod);
	e->m = 0;
	e->id = u.unf.numev++;

	/* we will update this if we find one history for this event which is
	 * not a cutoff */
	e->iscutoff = 1;

	/* add pe.comb.r->c to either the preset or context of e */
	if (pe.comb.ispre) {
		al_add (&e->pre, pe.comb.r->c);
		al_add (&pe.comb.r->c->post, e);
	} else {
		al_add (&e->cont, pe.comb.r->c);
		al_add (&pe.comb.r->c->cont, e);
	}

	/* set up preset and context */
	for (i = 0; i < pe.comb.size; i++) {
		c = pe.comb.tab[i].tab[pe.comb.tab[i].i]->c;
		if (pe.comb.tab[i].ispre) {
			al_add (&e->pre, c);
			al_add (&c->post, e);
		} else {
			al_add (&e->cont, c);
			al_add (&c->cont, e);
		}
	}

	ASSERT (e->pre.deg == t->pre.deg);
	ASSERT (e->cont.deg == t->cont.deg);

#ifdef CONFIG_DEBUG
	DPRINT ("+ Event e%d:%s; pre {", e->id, e->ft->name);
	for (i = e->pre.deg - 1; i >= 0; i--) {
		c = (struct cond *) e->pre.adj[i];
		DPRINT (" c%d:%s", c->id, c->fp->name);
	}
	DPRINT ("}; cont {");
	for (i = e->cont.deg - 1; i >= 0; i--) {
		c = (struct cond *) e->cont.adj[i];
		DPRINT (" c%d:%s", c->id, c->fp->name);
	}
	DPRINT ("}\n");
#endif

	/* therefore, at this the moment the event has no postset */
	return e;
}

static struct h * _pe_comb_new_hist (struct event *e)
{
	struct h *h;
	struct ec *r;
	int i;

	h = h_alloc (e);

	for (r = pe.comb.r; r->h == 0; r = r->r2) {
		ASSERT (r->r1);
		ASSERT (EC_ISREAD (r->r1));
		ASSERT (r->r1->h);
		h_add (h, r->r1->h);
	}
	h_add (h, r->h);
	al_add (&h->ecl, pe.comb.r);
	for (i = 0; i < pe.comb.size; i++) {
		ASSERT (pe.comb.tab[i].ispre ||
				EC_ISGEN (pe.comb.tab[i].tab[pe.comb.tab[i].i]));
		for (r = pe.comb.tab[i].tab[pe.comb.tab[i].i]; r->h == 0;
				r = r->r2) {
			ASSERT (r->r1);
			ASSERT (EC_ISREAD (r->r1));
			ASSERT (r->r1->h);
			h_add (h, r->r1->h);
		}
		h_add (h, r->h);
		al_add (&h->ecl, pe.comb.tab[i].tab[pe.comb.tab[i].i]);
	}

	/* compute the marking associated to that history, the marking hash,
	 * lists r(h), s(h), the size of the history and return */
	h_marking (h);

	return h;
}

static void _pe_comb_solution ()
{
	struct event *e;
	struct h *h;

#ifdef CONFIG_DEBUG
	int i;
	DPRINT ("  Solution %s, ", pe.comb.t->name);
	db_r2 (0, pe.comb.r, "");
	for (i = 0; i < pe.comb.size; i++) db_r2 (" ",
			pe.comb.tab[i].tab[pe.comb.tab[i].i], "");
	DPRINT ("\n");
#endif

	e = u.net.isplain ? 0 : _pe_comb_instance_of ();
	if (! e) e = _pe_comb_new_event ();

	h = _pe_comb_new_hist (e);
	if (h) _pe_q_insert (h);
}

static void _pe_comb_ent_add (int i, struct ec *r)
{
	ASSERT (i >= 0 && i < pe.comb.size);
	ASSERT (pe.comb.tab[i].nr <= pe.comb.tab[i].size);
	ASSERT (pe.comb.tab[i].size >= 1);
	ASSERT (pe.comb.tab[i].ispre || EC_ISGEN (r));

	/* make room for a new entry */
	if (pe.comb.tab[i].nr == pe.comb.tab[i].size) {
		pe.comb.tab[i].size <<= 1;
		pe.comb.tab[i].tab = gl_realloc (pe.comb.tab[i].tab,
				pe.comb.tab[i].size * sizeof (struct ec *));
	}

	/* append the entry at the end of the array */
	pe.comb.tab[i].tab[pe.comb.tab[i].nr] = r;
	pe.comb.tab[i].nr++;
}

static void _pe_comb_sort (void)
{
	int i, j;
	struct comb_entry e;

	for (i = 1; i < pe.comb.size; i++) {
		e = pe.comb.tab[i];
		for (j = i - 1; j >= 0; j--) {
			if (pe.comb.tab[j].nr <= e.nr) break;
			pe.comb.tab[j + 1] = pe.comb.tab[j];
		}
		pe.comb.tab[j + 1] = e;
	}

	for (i = 1; i < pe.comb.size; i++) {
		ASSERT (pe.comb.tab[i - 1].nr <= pe.comb.tab[i].nr);
	}
}

static void _pe_comb_init (struct ec *r, struct place *p, struct trans *t,
		int ispre)
{
	int i, m, idx;
	int  bits;
	struct place *pp;
	struct ec *rp;

	DPRINT ("  Explore  %s (%s", t->name, p->name);

	ASSERT (pe.comb.tab);
	ASSERT (r->c->fp == p);
	ASSERT (al_test (&t->pre, p) || al_test (&t->cont, p));

	m = ++u.mark;
	ASSERT (m > 0);

	pe.comb.size = 0;
	pe.comb.ispre = ispre;
	pe.comb.r = r;
	pe.comb.t = t;

	for (i = t->pre.deg - 1; i >= 0; i--) {
		pp = (struct place *) t->pre.adj[i];
		if (pp == p) continue;

		pp->m = m;
		pp->comb_idx = pe.comb.size;
		pe.comb.tab[pe.comb.size].nr = 0;
		pe.comb.tab[pe.comb.size].ispre = 1;
		pe.comb.size++;
		DPRINT (" %s", pp->name);
	}

	for (i = t->cont.deg - 1; i >= 0; i--) {
		pp = (struct place *) t->cont.adj[i];
		if (pp == p) continue;

		pp->m = m;
		pp->comb_idx = pe.comb.size;
		pe.comb.tab[pe.comb.size].nr = 0;
		pe.comb.tab[pe.comb.size].ispre = 0;
		pe.comb.size++;
		DPRINT (" %s", pp->name);
	}
	DPRINT (")\n");

	for (i = r->co.deg - 1; i >= 0; i--) {
		rp = (struct ec *) r->co.adj[i];
		bits = EC_BITS (rp);
		rp = EC_PTR (rp);
		if (rp->c->fp->m != m) continue;
		if (ispre && ! EC_BIT0 (bits)) continue;

		idx = rp->c->fp->comb_idx;
		if (pe.comb.tab[idx].ispre) {
			if (EC_BIT1 (bits)) {
				_pe_comb_ent_add (idx, rp);
			}
		} else {
			if (EC_ISGEN (rp)) {
				_pe_comb_ent_add (idx, rp);
			}
		}
	}

	_pe_comb_sort ();
}

static int _pe_comb_test (int i, int j)
{
	struct ec *r;
	struct ec *rp;
	int rispre, rpispre;

	r = pe.comb.tab[i].tab[pe.comb.tab[i].i];
	rispre = pe.comb.tab[i].ispre;
	rp = pe.comb.tab[j].tab[pe.comb.tab[j].i];
	rpispre = pe.comb.tab[j].ispre;

	return ec_conc_tst (r, EC_BITSET (rp, 2 * rpispre + rispre));
}

static void _pe_comb_explore (void)
{
	int i, j;

	if (pe.comb.size == 0) {
		_pe_comb_solution ();
		return;
	}

	i = 0;
	pe.comb.tab[0].i = pe.comb.tab[0].nr - 1;

	while (pe.comb.tab[0].i >= 0) {
		for (j = 0; j < i; j++) {
			if (! _pe_comb_test (i, j)) break;
		}

		if (j == i) {
			if (i + 1 == pe.comb.size) {
				_pe_comb_solution ();
				pe.comb.tab[i].i--;
			} else {
				i++;
				pe.comb.tab[i].i = pe.comb.tab[i].nr - 1;
			}
		} else {
			pe.comb.tab[i].i--;
		}

		while (i >= 1 && pe.comb.tab[i].i < 0) {
			i--;
			pe.comb.tab[i].i--;
		}
	}
}

void pe_init (void)
{
	int i, max;
	struct trans *t;
	struct ls *n;

	/* initialize the pe priority queue */
	pe.q.size = 0;
	pe.q.tab = 0;
	pe.q.skip = 0;

	/* compute the maximum preset + postset size in the input net */
	max = 0;
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);

		if (max < t->pre.deg + t->cont.deg) 
			max = t->pre.deg + t->cont.deg;
	}

	/* initialize the comb with proper sizes */
	max += 2;
	pe.comb.tab = gl_malloc (sizeof (struct comb_entry) * max);
	for (i = 0; i < max; i++) {
		pe.comb.tab[i].size = 4096;
		pe.comb.tab[i].tab = gl_realloc (0,
				4096 * sizeof (struct ec*));
	}
}

void pe_term (void)
{
	gl_free (pe.q.tab);
	gl_free (pe.comb.tab);
}

struct h * pe_pop (void)
{
	struct h * ret;
	int idx, nidx;
	struct h * lst;

	/* nothing to do if the queue is empty */
	if (pe.q.size == 0) return 0;

	/* the minimal element is at offset 1 in the tab */
	ret = pe.q.tab[1];

	/* get a pointer to the last element and decrement size of the queue */
	lst = pe.q.tab[pe.q.size];
	pe.q.size--;

	/* we want to fill position 1 in the queue */
	idx = 1;
	while (1) {

		/* compute the index of the child of idx with the minimum
		 * weight */
		nidx = idx * 2;
		if (nidx > pe.q.size) break;
		if (nidx < pe.q.size && h_cmp (pe.q.tab[nidx],
				pe.q.tab[nidx + 1]) > 0) nidx++;

		/* if last smaller than (or = to) the minimum, we can stop */
		if (h_cmp (lst, pe.q.tab[nidx]) <= 0) break;

		/* otherwise, tab[nidx] is below lst and its sibling */
		pe.q.tab[idx] = pe.q.tab[nidx];
		idx = nidx;
	}
	pe.q.tab[idx] = lst;

	return ret;
}

#ifdef CONFIG_DEBUG
static void __pe_debug (struct ec *r)
{
	int i;

	db_r2 ("+ Condition ", r, " type ");
	DPRINT ("%s co: \n", EC_ISCOMP (r) ? "C" :
			EC_ISREAD (r) ? "R" : "G");
	
	for (i = r->co.deg - 1; i >= 0; i--) db_r2 ("   ", r->co.adj[i], 0);

	DPRINT ("  rco:\n");
	for (i = r->rco.deg - 1; i >= 0; i--) db_r2 ("   ", r->rco.adj[i], 0);
}
#else
#define __pe_debug(r)
#endif


void pe_update_gen (struct ec * r)
{
	struct place *p;
	struct trans *t;
	int i;

	ASSERT (r);
	ASSERT (EC_ISGEN (r));
	__pe_debug (r);

	p = r->c->fp;
	for (i = p->post.deg - 1; i >= 0; i--) {
		t = (struct trans *) p->post.adj[i];
		_pe_comb_init (r, p, t, 1);
		_pe_comb_explore ();
	}

	for (i = p->cont.deg - 1; i >= 0; i--) {
		t = (struct trans *) p->cont.adj[i];
		_pe_comb_init (r, p, t, 0);
		_pe_comb_explore ();
	}
}

void pe_update_read (struct ec * r)
{
	struct place *p;
	struct trans *t;
	int i;

	ASSERT (r);
	ASSERT (EC_ISREAD (r) || EC_ISCOMP (r));
	__pe_debug (r);

	p = r->c->fp;
	for (i = p->post.deg - 1; i >= 0; i--) {
		t = (struct trans *) p->post.adj[i];
		_pe_comb_init (r, p, t, 1);
		_pe_comb_explore ();
	}
}

void pe_skip (void)
{
	pe.q.skip = 1;
}
