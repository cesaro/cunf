
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
#include "debug.h"
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
	al_init (&r->rco);
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
	struct ec *r, *rp;
	int i;

	r = _ec_alloc (c);
	r->h = h;
	r->r2 = 0;

	/* search for the ancestor if the new r is reading */
	/* FIXME -- there is a better way of doing this: this function
	 * receives and stores the ancestor; in _pe_comb_new_hist we
	 * explicity mark the ecs in the context and _unfold_enriched
	 * iterates through them, calling here */
	rp = 0;
	if (c->pre != h->e) {
		for (i = h->ecl.deg - 1; i >= 0; i--) {
			rp = (struct ec *) h->ecl.adj[i];
			if (rp->c == c) break;
		}
		ASSERT (i >= 0);
		ASSERT (EC_ISGEN (rp));
#ifdef CONFIG_DEBUG
		db_r2 ("  ", rp, "");
		db_r2 (" is ancestor of ", r, 0);
#endif
		r->r1 = rp;
	} else {
		r->r1 = 0;
	}
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
	static int m = 0;
	static struct ec *lastr = 0;
	int i;

	/* r is a reading ec.; rp is a reading or compound ec. associated to
	 * the same condition; check whether one of the readers in rp is
	 * included in r's history and return 1 if yes; in such case, there is
	 * another compound ec which can be combined with r to produce a
	 * genuine compound ec.  To check that, it is enough to mark r(r->h),
	 * not r->h */

	ASSERT (EC_ISREAD (r) && ! EC_ISCOMP (r));
	ASSERT (EC_ISREAD (rp) || EC_ISCOMP (rp));
	ASSERT (r->c == rp->c);

	/* mark r(r->h) only in the first call */
	if (lastr != r) {
		lastr = r;
		m = ++u.mark;
		ASSERT (m > 0);
		for (i = r->h->rd.deg - 1; i >= 0; i--) {
			((struct event *) r->h->rd.adj[i])->m = m;
		}
	}

	for (; rp->h == 0; rp = rp->r2) {
		ASSERT (rp->r1);
		ASSERT (EC_ISREAD (rp->r1));
		ASSERT (rp->r1->h);
		if (rp->r1->h->e->m == m) return 1;
	}
	if (rp->h->e->m == m) return 1;
	return 0;
}

int __attribute__ ((noinline)) _ec_conc_compound_00 (struct ec *r, struct ec *rp)
{
	static struct ec *lastr = 0;
	struct event *e;
	struct al *rd;
	static int m;
	int i, j;

	ASSERT (r);
	ASSERT (EC_ISCOMP (r));
	rp = EC_PTR (rp);

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
			if (e->cont.adj[j] == r->c) return 0;
		}
	}

	return 1;
}

void _ec_conc_compound (struct ec *r)
{
	struct ec *rp;
	int x, i, bit0, bit1;
	static int m = 0;
	static struct ec *lastr1 = 0;

	/* r is r1 \cup r2
	 * r || r' iff r1 || r' and r2 || r'
	 * r // r' iff r || r' and cont(c) \cap H' \subseteq H (algorithm)
	 * r' // r iff r' // r1 and r' // r2 */

	ASSERT (EC_PTR (r) == r);
	ASSERT (r->h == 0);
	ASSERT (r->r1);
	ASSERT (EC_ISREAD (r->r1));
	ASSERT (EC_ISGEN (r->r1->r1));
	ASSERT (r->r2);
	ASSERT (r->r1->co.deg >= 1);
	ASSERT (r->r2->co.deg >= 1);

	/* optmz: if the last call was for an enriched condition derived from
	 * the same reading condition r->r1, the array r->r1->co is already
	 * marked :) */
	if (lastr1 != r->r1) {
		lastr1 = r->r1;

		/* generate four marks */
		m = ++u.mark;
		u.mark += 3;
		ASSERT (m > 0);

		/* mark with m, m+1, m+2 or m+3 in co(r1) depending on EC_BITS
		 * (rp) */
		for (i = r->r1->co.deg - 1; i >= 0; i--) {
			rp = (struct ec *) r->r1->co.adj[i];
			ASSERT (EC_BITS (rp) <= 3);
			EC_PTR (rp)->m = m + EC_BITS (rp);
		}
	}

	/* store r in r->ancestor->rco */
	al_add (&r->r1->r1->rco, r);

	/* enumerate co(r2) and set up the asymmetric concurrency
	 * appropriately */
	for (i = r->r2->co.deg - 1; i >= 0; i--) {
		rp = (struct ec *) r->r2->co.adj[i];

		/* recover the bits in co(r1) and compute the bits for co (r) */
		x = EC_PTR (rp)->m - m;
		if (x < 0 || x > 3) continue;

		bit0 = (x & 1) || EC_BIT0 (rp) || _ec_conc_compound_00 (r, rp);
		bit1 = (x & 2) && EC_BIT1 (rp);

		al_add (&r->co, EC_BITSET (rp, 2 * bit1 + bit0));
		al_add (&EC_PTR(rp)->co, EC_BITSET (r, 2 * bit0 + bit1));
	}

	u.unf.numco += r->co.deg * 2;
	u.unf.numrco++;
}

void __attribute__ ((noinline)) _ec_conc_siblings (struct ec *r)
{
	struct ec *rp;
	struct event *e;
	struct cond *c;
	//struct ls *n;
	int i;

	/* set r || rp, and r // rp, and rp // r for all rp sharing the same
	 * history than r, including r */

	if (EC_ISGEN (r) && r->c->fp->cont.deg) al_add (&r->rco, r);

	e = r->h->e;
	for (i = e->post.deg - 1; i >= 0; i--) {
		c = e->post.adj[i];
		if (c == r->c) continue;
		if (c->ecl.next == 0) continue;
		rp = ls_i (struct ec, c->ecl.next, nod);
		if (rp->h == r->h) {
			al_add (&r->co, EC_BITSET (rp, 3));
			al_add (&rp->co, EC_BITSET (r, 3));
		} /* else {
			for (n = c->ecl.next; n; n = n->next) {
				rp = ls_i (struct ec, n, nod);
				ASSERT (rp->h != r->h);
			}
		} */
	}

	for (i = e->cont.deg - 1; i >= 0; i--) {
		c = e->cont.adj[i];
		if (c == r->c) continue;
		if (c->ecl.next == 0) continue;
		rp = ls_i (struct ec, c->ecl.next, nod);
		if (rp->h == r->h) {
			al_add (&r->co, EC_BITSET (rp, 3));
			al_add (&rp->co, EC_BITSET (r, 3));
		} /* else {
			for (n = c->ecl.next; n; n = n->next) {
				rp = ls_i (struct ec, n, nod);
				ASSERT (rp->h != r->h);
			}
		} */
	}
}

void _ec_conc_intersec_mrk (struct ec *ri, register int m, int *nr, int rco)
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
	if (rco) {
		for (j = ri->rco.deg - 1; j >= 0 && k; j--) {
			rp = EC_PTR (ri->rco.adj[j]);
			if (rp->m == m1) {
				rp->m = m;
				k--;
			}
		}
	}
	*nr -= k;
}

void _ec_conc_intersec_sort (struct ec *r, int mblack)
{
	int i, j;
	struct ec *e;

#define A(i) r->h->ecl.adj[(i)]
#define K(e) (((struct ec *) (e))->co.deg + \
		((struct ec *) (e))->c->m != mblack ? ((struct ec *) (e))->rco.deg : 0)

	for (i = 1; i < r->h->ecl.deg; i++) {
		e = A (i);
		for (j = i - 1; j >= 0; j--) {
			if (K (A (j)) >= K (e)) break;
			A (j + 1) = A (j);
		}
		A (j + 1) = e;
	}

	//PRINT ("c%d h%d: x%dx %d", r->c->id, r->h->id, r->h->ecl.deg, K (A (0)));
	for (i = 1; i < r->h->ecl.deg; i++) {
		ASSERT (K (A (i - 1)) >= K (A (i)));
		//PRINT (" %d", K (A (i)));
	}
	//PRINT ("\n");
#undef K
#undef A
}

int _ec_conc_intersec_mrk_fst (struct ec *r, register int m, int rco)
{
	register int j;

	for (j = r->co.deg - 1; j >= 0; j--) EC_PTR (r->co.adj[j])->m = m;
	if (rco) {
		for (j = r->rco.deg - 1; j >= 0; j--) {
			EC_PTR (r->rco.adj[j])->m = m;
		}
		return r->co.deg + r->rco.deg;
	}
	return r->co.deg;
}

int __attribute__ ((noinline)) _ec_conc_intersec (struct ec *r, int *nr, int mblack)
{
	int i, m;
	struct ec *ri;

	/* we need to intersect something if there is at least two sets */
	if (r->h->ecl.deg == 1) {
		*nr = 0;
		return 0;
	}

	/* sort conditions r_0 .. r_{n-1} by decreasing size of co(r_i) */
	_ec_conc_intersec_sort (r, mblack);

	/* mark r_{n-1} */
	m = ++u.mark;
	ASSERT (m > 0);
	i = r->h->ecl.deg - 1;
	ri = (struct ec *) r->h->ecl.adj[i];
	*nr = _ec_conc_intersec_mrk_fst (ri, m, ri->c->m != mblack);

	/* intersection for conditions r_{n-2} to r_1 */
	for (i--; i >= 1; i--) {
		ri = (struct ec *) r->h->ecl.adj[i];
		m = ++u.mark;
		ASSERT (m > 0);
		_ec_conc_intersec_mrk (ri, m, nr, ri->c->m != mblack);
	}
	return m;
}

void __attribute__ ((noinline)) _ec_conc_bgr (struct ec *r, int mblack, int mgreen, int mred)
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

	/* mark in red r(H) and any condition read by those events (if it's not
	 * in black), counting how many red events read it */
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
		}
	}
}

void __attribute__ ((noinline)) _ec_conc_add (struct ec *r, struct ec *rp,
		int mblack, int mgreen, int mred)
{
	struct event * e;
	struct cond *c;
	struct al *rd;
	int i, j, x, bit0, bit1;

	if (rp->c->m == mblack) return;
	rd = EC_ISCOMP (rp) ? &rp->rd : &rp->h->rd;

	if (rp->c == r->c) {
		for (i = rd->deg - 1; i >= 0; i--) {
			e = (struct event *) rd->adj[i];
			if (e->m == mgreen) return;
		}
		ASSERT (EC_ISREAD (r));
		ASSERT (EC_ISREAD (rp) || EC_ISCOMP (rp) || rp == r->r1);
		if (rp == r->r1) {
			al_add (&rp->rco, r);
			u.unf.numrco++;
		} else {
			al_add (&r->rco, rp);
		}
		return;
	}

	if (rp->c->m == mred) {
		bit0 = 1;
		x = 0;
		if (r->c->cont.deg == 0) {
			for (i = rd->deg - 1; i >= 0; i--) {
				e = (struct event *) rd->adj[i];
				if (e->m == mgreen) return;
				if (e->m == mred) {
					for (j = e->cont.deg - 1; j >= 0; j--) {
						c = (struct cond *) e->cont.adj[j];
						if (c == rp->c) { x++; break; }
					}
				}
			}
		} else {
			for (i = rd->deg - 1; i >= 0 && bit0; i--) {
				e = (struct event *) rd->adj[i];
				if (e->m == mgreen) return;
				if (e->m == mred) {
					for (j = e->cont.deg - 1; j >= 0; j--) {
						c = (struct cond *) e->cont.adj[j];
						if (c == rp->c) { x++; break; }
					}
				} else {
					for (j = e->cont.deg - 1; j >= 0; j--) {
						c = (struct cond *) e->cont.adj[j];
						if (c == r->c) { bit0 = 0; break; }
					}
				}
			}
			for (; i >= 0; i--) {
				e = (struct event *) rd->adj[i];
				if (e->m == mgreen) return;
				if (e->m == mred) {
					for (j = e->cont.deg - 1; j >= 0; j--) {
						c = (struct cond *) e->cont.adj[j];
						if (c == rp->c) { x++; break; }
					}
				}
			}
		}
		bit1 = rp->c->cnt == x;
	} else {
		bit1 = 1;
		bit0 = 1;
		if (r->c->cont.deg == 0) {
			for (i = rd->deg - 1; i >= 0; i--) {
				e = (struct event *) rd->adj[i];
				if (e->m == mgreen) return;
			}
		} else {
			for (i = rd->deg - 1; i >= 0 && bit0; i--) {
				e = (struct event *) rd->adj[i];
				if (e->m == mgreen) return;
				if (e->m == mred) continue;
				for (j = e->cont.deg - 1; j >= 0; j--) {
					c = (struct cond *) e->cont.adj[j];
					if (c == r->c) { bit0 = 0; break; }
				}
			}
			for (; i >= 0; i--) {
				e = (struct event *) rd->adj[i];
				if (e->m == mgreen) return;
			}
		}
	}

	al_add (&r->co, EC_BITSET (rp, 2 * bit1 + bit0));
	al_add (&rp->co, EC_BITSET (r, 2 * bit0 + bit1));
}

#if 0
/* this is the original version.  Important: this function must not be inlined,
 * since _ec_conc_last must use all available registers to be fast (it is
 * usually the bottleneck.  The version above is an optimized one for several
 * conditions known a priori
 * mercredi 30 novembre 2011, 17:53:46 (UTC+0100)  The code below seems not to
 * be in sync with the code above, to be use carefully!
 */
static void _ec_conc_add (struct ec *r, struct ec *rp,
		int mblack, int mgreen, int mred)
{
	struct event * e;
	struct cond *c;
	struct al *rd;
	int i, j, x, bit0, bit1;
	struct ec *ranc;

	if (rp->c->m == mblack) return;

	bit0 = 1;
	x = 0;
	rd = EC_ISCOMP (rp) ? &rp->rd : &rp->h->rd;
	for (i = rd->deg - 1; i >= 0; i--) {
		e = (struct event *) rd->adj[i];
		if (e->m == mgreen) return;
		if (e->m == mred) {
			for (j = e->cont.deg - 1; j >= 0; j--) {
				c = (struct cond *) e->cont.adj[j];
				if (c == rp->c) {
					x++;
					break;
				}
			}
		} else {
			for (j = e->cont.deg - 1; j >= 0; j--) {
				c = (struct cond *) e->cont.adj[j];
				if (c == r->c) {
					bit0 = 0;
					break;
				}
			}
		}
	}

	if (rp->c == r->c) {
		ASSERT (EC_ISREAD (r));
		ASSERT (EC_ISREAD (rp) || EC_ISCOMP (rp));
		ranc = EC_ISREAD (rp) ? r->r1 : r->r1->r1;
		ASSERT (ranc);
		if (rp == ranc) {
			al_add (&ranc->rco, r);
			u.unf.numrco++;
		} else {
			al_add (&r->rco, rp);
		}
	} else {
		bit1 = rp->c->m != mred || rp->c->cnt == x;
		al_add (&r->co, EC_BITSET (rp, 2 * bit1 + bit0));
		al_add (&rp->co, EC_BITSET (r, 2 * bit0 + bit1));
	}
}
#endif

void __attribute__ ((noinline)) _ec_conc_last (struct ec *r, register int m,
		register int nr, int mblack, int mgreen, int mred)
{
	register struct ec * r0;
	register struct ec * rp;
	register int i;

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
		if (r0->c->m != mblack) {
			for (i = r0->rco.deg - 1; i >= 0 && nr; i--) {
				rp = EC_PTR (r0->rco.adj[i]);
				if (rp->m == m) {
					_ec_conc_add (r, rp, mblack, mgreen, mred);
					nr--;
				}
			}
		}
	} else {
		for (i = r0->co.deg - 1; i >= 0; i--) {
			_ec_conc_add (r, EC_PTR (r0->co.adj[i]), mblack,
					mgreen, mred);
		}
		if (r0->c->m != mblack) {
			for (i = r0->rco.deg - 1; i >= 0; i--) {
				_ec_conc_add (r, EC_PTR (r0->rco.adj[i]), mblack,
						mgreen, mred);
			}
		}
	}
}

void ec_conc (struct ec *r)
{
	int m, nr, mblack, mgreen, mred;

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
	if (r->h->id == 0) {
		u.unf.numco += r->co.deg * 2;
		u.unf.numrco += r->rco.deg;
		return;
	}
	ASSERT (r->h->ecl.deg >= 1);

	/* 3. colorize certain events and conditions */
	mblack = ++u.mark;
	mgreen = ++u.mark;
	mred = ++u.mark;
	ASSERT (mblack > 0 && mgreen > 0 && mred > 0);
	_ec_conc_bgr (r, mblack, mgreen, mred);

	/* 4. intersect conc. lists for r_1 ... r_{n-1}; m will be 0 if n = 1 */
	m = _ec_conc_intersec (r, &nr, mblack);

	/* 6. consider conditions in co(r_0) \cup rco(r_0) that has been marked
	 * in _ec_conc_intersec, or all conditions if m = 0, and include them
	 * in {r}co(r) and {r}co(rp) */
	_ec_conc_last (r, m, nr, mblack, mgreen, mred);

	u.unf.numco += r->co.deg * 2;
	u.unf.numrco += r->rco.deg;
}

int ec_conc_tst (register struct ec *r, register struct ec *rp)
{
	struct ec *auxr;
	register unsigned long  mask;
	register int i;

	ASSERT (EC_PTR (r) == r);

	mask = EC_BITS (rp);
	if (r->co.deg > EC_PTR (rp)->co.deg) {
		mask = 2 * (mask & 1) + ((mask & 2) >> 1);
		auxr = r;
		r = EC_PTR (rp);
		rp = EC_BITSET (auxr, mask);
	}

	if (mask == 3) {
		for (i = r->co.deg - 1; i >= 0; i--) {
			if (r->co.adj[i] == rp) return 1;
		}
	} else {
		mask = ~0 - 3 + mask;
		for (i = r->co.deg - 1; i >= 0; i--) {
			if ((((unsigned long) r->co.adj[i]) & mask) == 
						(unsigned long) rp) return 1;
		}
	}
	return 0;
}
