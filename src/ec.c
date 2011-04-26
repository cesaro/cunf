
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
#include "ls/ls.h"
#include "al/al.h"
#include "glue.h"
#include "ec.h"
#include "pe.h"
#include "h.h"

static struct ec * _ec_alloc (struct cond * c)
{
	struct ec * r;

	/* mallocate a new structure */
	r = gl_malloc (sizeof (struct ec));
	ASSERT (((unsigned long) r & 7) == 0);

	/* important, append *to the head* to the list c->ecl */
	ls_insert (&c->ecl, &r->nod);

	r->c = c;
	al_init (&r->co);
	al_init (&r->rd);
	r->m = 0;
	return r;
}

static void _ec_uniq_rd (struct ec *r, struct ec *r1, struct ec *r2)
{
	struct event *e;
	int i, m;
	struct al *rd2;

	ASSERT (EC_ISREAD (r1));
	ASSERT (EC_ISREAD (r2) || EC_ISCOMP (r2));

	m = ++u.mark;
	ASSERT (m > 0);

	al_cpy (&r->rd, &r1->h->rd);
	for (i = r1->h->rd.deg - 1; i >= 0; i--) {
		((struct event *) r1->h->rd.adj[i])->m = m;
	}

	rd2 = EC_ISCOMP (r2) ? &r2->rd : &r2->h->rd;
	for (i = rd2->deg - 1; i >= 0; i--) {
		e = (struct event *) rd2->adj[i];
		if (e->m != m) {
			e->m = m;
			al_add (&r->rd, e);
		}
	}
}

struct ec * ec_alloc (struct cond * c, struct h * h)
{
	struct ec *r;

	r = _ec_alloc (c);
	r->h = h;
	r->r1 = r->r2 = 0;
	return r;
}

struct ec * ec_alloc2 (struct ec *r1, struct ec *r2)
{
	struct ec *r;

	ASSERT (r1->c == r2->c);
	r = _ec_alloc (r1->c);
	r->h = 0;
	r->r1 = r1;
	r->r2 = r2;
	_ec_uniq_rd (r, r1, r2);
	return r;
}

int ec_included (struct ec *r, register struct ec *rp)
{
	register int m;

	/* check whether r's history is included in rp's history */

	ASSERT (EC_ISREAD (r) && ! EC_ISCOMP (r));
	ASSERT (EC_ISREAD (rp) || EC_ISCOMP (rp));

	m = ++u.mark;
	ASSERT (m > 0);
	h_mark (r->h, m);

	for (; rp->h == 0; rp = rp->r2) {
		ASSERT (rp->r1);
		ASSERT (EC_ISREAD (rp->r1));
		ASSERT (rp->r1->h);
		if (rp->r1->h->m == m) return 1;
	}
	if (rp->h->m == m) return 1;
	return 0;
}

static void _ec_conc_compound_00 (struct ec *r, struct ec *rp)
{
	static struct ec *lastr = 0;
	struct event *e;
	struct al *rd;
	static int m;
	int i, j;

	ASSERT (r);
	ASSERT (EC_ISCOMP (r));

	if (lastr != r) {
		lastr = r;
		m = ++u.mark;
		ASSERT (m > 0);

		for (i = r->rd.deg - 1; i >= 0; i--) {
			e = r->rd.adj[i];
			for (j = e->cont.deg - 1; j >= 0; j--) {
				if (e->cont.adj[j] == r->c) { 
					e->m = m;
					break;
				}
			}
		}
	}

	rd = EC_ISCOMP (rp) ? &rp->rd : &rp->h->rd;
	for (i = rd->deg - 1; i >= 0; i--) {
		e = (struct event *) rd->adj[i];
		if (e->m == m) continue;
		for (j = e->cont.deg - 1; j >= 0; j--) {
			if (e->cont.adj[j] == r->c) {
				al_add (&r->co, EC_BITSET (rp, 0));
				return;
			}
		}
	}

	al_add (&r->co, EC_BITSET (rp, 1));
}

static int _ec_conc_compound_r1andr2 (struct ec *r1, struct ec *r2,
	       	struct ec *rp)
{
	int i;

	ASSERT (EC_PTR (rp) == rp);
	ASSERT (EC_PTR (r1) == r1);
	ASSERT (EC_PTR (r2) == r2);

	r1 = EC_BITSET (r1, 1);
	r2 = EC_BITSET (r2, 1);

	for (i = rp->co.deg - 1; i >= 0; i--) {
		if (rp->co.adj[i] == r1) {
			for (; i >= 0; i--) {
				if (rp->co.adj[i] == r2) return 1;
			}
			return 0;
		}
		if (rp->co.adj[i] == r2) {
			for (; i >= 0; i--) {
				if (rp->co.adj[i] == r1) return 1;
			}
			return 0;
		}
	}
	return 0;
}

static void _ec_conc_compound (struct ec *r)
{
	struct ec *rp;
	int i, m, mbit, bit;

	/* r is r1 \cup r2
	 * r || r' iff r1 || r' and r2 || r'
	 * r // r' iff r || r' and cont(c) \cap H' \subseteq H (algorithm)
	 * r' // r iff r' // r1 and r' // r2 */

	ASSERT (EC_PTR (r) == r);
	ASSERT (r->h == 0);
	ASSERT (r->r1);
	ASSERT (r->r2);
	ASSERT (r->r1->co.deg >= 1);
	ASSERT (r->r2->co.deg >= 1);

	/* generate two marks */
	m = ++u.mark;
	ASSERT (m > 0);
	mbit = ++u.mark;
	ASSERT (mbit > 0);

	al_add (&r->co, EC_BITSET (r, 1));
	
	/* mark with m or mbit in co(r1) depending on EC_BIT (rp) */
	for (i = r->r1->co.deg - 1; i >= 0; i--) {
		rp = (struct ec *) r->r1->co.adj[i];
		EC_PTR (rp)->m = m + EC_BIT (rp);
		//ASSERT (EC_BIT (rp) == 0 || EC_BIT (rp) == 1);
	}

	/* enumerate co(r2) and set up the asymmetric concurrency
	 * appropriately */
	for (i = r->r2->co.deg - 1; i >= 0; i--) {
		rp = (struct ec *) r->r2->co.adj[i];

		if (EC_PTR (rp)->m == mbit) {
			al_add (&r->co, EC_BITSET (rp, 1));
			bit = _ec_conc_compound_r1andr2 (r->r1, r->r2, EC_PTR(rp));
			al_add (&(EC_PTR (rp)->co), EC_BITSET (r, bit));
		} else if (EC_PTR (rp)->m == m) {
			bit = _ec_conc_compound_r1andr2 (r->r1, r->r2, EC_PTR(rp));
			al_add (&(EC_PTR (rp)->co), EC_BITSET (r, bit));
			if (EC_BIT (rp)) {
				al_add (&r->co, EC_BITSET (rp, 1));
			} else {
				ASSERT (EC_PTR (rp) == rp);
				_ec_conc_compound_00 (r, rp);
			}
		}
	}
}

static void _ec_conc_siblings (struct ec *r)
{
	struct ec *rp;
	struct event *e;
	struct cond *c;
	struct ls *n;
	int i;

	/* set r || rp, and r // rp, and rp // r for all rp sharing the same
	 * history than r, including r */

	al_add (&r->co, EC_BITSET (r, 1));

	e = r->h->e;
	for (i = e->post.deg - 1; i >= 0; i--) {
		c = e->post.adj[i];
		if (c == r->c) continue;
		for (n = c->ecl.next; n; n = n->next) {
			rp = ls_i (struct ec, n, nod);
			if (rp->h == r->h) {
				al_add (&r->co, EC_BITSET (rp, 1));
				al_add (&rp->co, EC_BITSET (r, 1));
			}
		}
	}

	for (i = e->cont.deg - 1; i >= 0; i--) {
		c = e->cont.adj[i];
		if (c == r->c) continue;
		for (n = c->ecl.next; n; n = n->next) {
			rp = ls_i (struct ec, n, nod);
			if (rp->h == r->h) {
				al_add (&r->co, EC_BITSET (rp, 1));
				al_add (&rp->co, EC_BITSET (r, 1));
			}
		}
	}
}

static void _ec_conc_intersec_mrk (struct ec *ri, register int m, int *nr)
{
	register int j;
	register int k;
	register struct ec *rp;
	register int m1;

	k = *nr;
	m1 = m - 1;
	for (j = ri->co.deg - 1; j >= 0 && k; j--) {
		rp = EC_PTR (ri->co.adj[j]);
		if (rp->m == m1) {
			rp->m = m;
			k--;
		}
	}
	*nr -= k;
}

static int _ec_conc_intersec_mrk_r1 (struct ec *r1, register int m)
{
	register int j;

	for (j = r1->co.deg - 1; j >= 0; j--) EC_PTR (r1->co.adj[j])->m = m;
	return r1->co.deg;
}

static int _ec_conc_intersec (struct ec *r, int *nr)
{
	int i, m;
	struct ec *ri;

	/* we need to intersect something if there is at least two sets */
	if (r->h->ecl.deg == 1) {
		*nr = 0;
		return 0;
	}

	/* mark r_1 */
	m = ++u.mark;
	ASSERT (m > 0);
	*nr = _ec_conc_intersec_mrk_r1 ((struct ec *) r->h->ecl.adj[1], m);

	/* intersection for conditions r_2 to r_{n-1} */
	for (i = r->h->ecl.deg - 1; i >= 2; i--) {
		ri = (struct ec *) r->h->ecl.adj[i];
		m = ++u.mark;
		ASSERT (m > 0);
		_ec_conc_intersec_mrk (ri, m, nr);
	}
	return m;
}

static void _ec_conc_bgr (struct ec *r, int mblack, int mgreen, int mred)
{
	struct event *e;
	struct cond *c;
	int i, j;

	e = r->h->e;
	/* mark in black pre(e); in green cont (pre (e)) */
	for (i = e->pre.deg - 1; i >= 0; i--) {
		c = (struct cond *) e->pre.adj[i];
		c->m = mblack;
		for (j = c->cont.deg - 1; j >= 0; j--) {
			((struct event *) c->cont.adj[j])->m = mgreen;
		}
	}

	/* remove the green from those e' *in H* that read from pre (e) */
	for (i = r->h->sd.deg - 1; i >= 0; i--) {
		((struct event *) r->h->sd.adj[i])->m = 0;
	}

	/* mark in red r(H) and any condition read by those events, counting
	 * how many red events read it */
	for (i = r->h->rd.deg - 1; i >= 0; i--) {
		e = (struct event *) r->h->rd.adj[i];
		e->m = mred;
		for (j = e->cont.deg - 1; j >= 0; j--) {
			c = (struct cond *) e->cont.adj[j];
			if (c->m == mred) {
				c->cnt++;
			} else {
				if (c->m != mblack) {
					c->cnt = 1;
					c->m = mred;
				}
			}
			/* if (r->c->id == 1163 && r->h->id == 749) {
				if (c->id == 58) DEBUG ("(c1163,h749): e%d reads c58; cnt %d", e->id, c->cnt);
			} */
		}
	}
}

static void _ec_conc_add (struct ec *r, struct ec *rp, int mblack, int mgreen,
		int mred)
{
	struct event * e;
	struct cond *c;
	struct al *rd;
	int i, j, x, bit;

	//if (rp->c->m == mblack) db_r2 ("Discarding ", rp, " because black\n");
	if (rp->c->m == mblack) return;

	bit = 1;
	x = 0;
	rd = EC_ISCOMP (rp) ? &rp->rd : &rp->h->rd;
	for (i = rd->deg - 1; i >= 0; i--) {
		e = (struct event *) rd->adj[i];
		//if (e->m == mgreen) db_r2 ("Discarding ", rp, " because green\n");
		if (e->m == mgreen) return;
		if (e->m == mred) {
			for (j = e->cont.deg - 1; j >= 0; j--) {
				c = (struct cond *) e->cont.adj[j];
				if (c == rp->c) {
					x += 1;
					/* if (rp->c->id == 58 && rp->h && rp->h->id == 351) {
						DEBUG ("(c58,h351): e%d red and reads c%d; x %d", e->id, c->id, x);
					} */
					break;
				}
			}
		} else {
			for (j = e->cont.deg - 1; j >= 0; j--) {
				c = (struct cond *) e->cont.adj[j];
				if (c == r->c) {
					bit = 0;
					break;
				}
			}
		}
	}

	/* if (rp->c->id == 58 && rp->h && rp->h->id == 351) {
		TRACE (bit, "d");
		TRACE (x, "d");
		TRACE (rp->c->m, "d");
		TRACE (mred, "d");
		TRACE (rp->c->cnt, "d");
		TRACE (rp->c->m == mred && rp->c->cnt != x ? 0 : 1, "d");
	} */
	al_add (&r->co, EC_BITSET (rp, bit));
	bit = rp->c->m == mred && rp->c->cnt != x ? 0 : 1;
	al_add (&rp->co, EC_BITSET (r, bit));
}

void ec_conc (struct ec *r)
{
	int  mblack, mgreen, mred;

	int nr;
	register int i;
	register int m;
	register struct ec *r0;
	register struct ec *rp;

	/* 0. we process here only generating or reading ecs. */
	ASSERT (EC_PTR (r) == r);
	if (EC_ISCOMP (r)) return _ec_conc_compound (r);

	/* 1. take care of the sibling enriched conditions (those with the
	 * same history) */
	ASSERT (r->c);
	ASSERT (r->h);
	_ec_conc_siblings (r);

	/* 2. we now deal with histories H' smaller than H in \prec, if they
	 * exist */
	if (r->h->id == 0) return;
	ASSERT (r->h->ecl.deg >= 1);

	/* 3. intersect lists co(r_1) .. co(r_{n-1}); m will be 0 if n = 1 */
	m = _ec_conc_intersec (r, &nr);

	/* 4. mark in black conditions in pre(e); mark in green events in
	 * cont(pre(e)) */
	mblack = ++u.mark;
	mgreen = ++u.mark;
	mred = ++u.mark;
	ASSERT (mblack > 0 && mgreen > 0 && mred > 0);

	/* 5. colorize certain events and conditions */
	_ec_conc_bgr (r, mblack, mgreen, mred);

	/* 6. consider conditions in co(r_0) that has been marked in
	 * _ec_conc_intersec, or all conditions if m = 0, and include them in
	 * co(r) and co(rp)  */
	r0 = (struct ec *) r->h->ecl.adj[0];
	ASSERT (r0);
	if (m) {
		for (i = r0->co.deg - 1; i >= 0 && nr; i--) {
			rp = EC_PTR (r0->co.adj[i]);
			if (rp->m == m) {
				_ec_conc_add (r, rp, mblack, mgreen, mred);
				nr--;
			}
		}
	} else {
		for (i = r0->co.deg - 1; i >= 0; i--) {
			_ec_conc_add (r, EC_PTR (r0->co.adj[i]), mblack,
					mgreen, mred);
		}
	}
}

int ec_conc_tst (struct ec *r, register struct ec *rp)
{
	register struct ec *rpp;
	register struct al *l;
	register int i;

	rpp = EC_BITSET (rp, 1);

	ASSERT (EC_PTR (r) == r && EC_PTR (rp) == rp);
	ASSERT ((al_test (&r->co, rp) || al_test (&r->co, rpp)) ==
			(al_test (&rp->co, r) || al_test (&rp->co, rpp)));

	l = r->co.deg < rp->co.deg ? &r->co : &rp->co;
	for (i = l->deg - 1; i >= 0; i--) {
		if (l->adj[i] == rp || l->adj[i] == rpp) return 1;
	}
	return 0;
}

int ec_asymconc_tst (register struct ec *r, register struct ec *rp)
{
	register int i;

	ASSERT (EC_PTR (r) == r && EC_PTR (rp) == rp);
	rp = EC_BITSET (rp, 1);
	for (i = r->co.deg - 1; i >= 0; i--) if (r->co.adj[i] == rp) return 1;
	return 0;
}
