
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
#include "ec.h"
#include "co.h"
#include "h.h"

static void _co_add_compound (struct ec *r)
{
	struct ec *rp;
	int i;
	static int m = 0;
	static struct ec *r1 = 0;

	/* r is concurrent to r' iff all ec r_1 ... r_n defining the compound
	 * ec. r are concurrent to r' */
	ASSERT (r->h == 0);
	ASSERT (r->r1);
	ASSERT (r->r2);
	ASSERT (r->r1->co.deg >= 1);
	ASSERT (r->r2->co.deg >= 1);

	/* optmz: if the last call was for an enriched condition derived from
	 * the same reading condition r->r1, the array r->r1->co is already marked :)
	 */
	if (r1 != r->r1) {
		r1 = r->r1;
		m = ++u.mark;
		ASSERT (m > 0);
	
		al_add (&r->co, r);
		
		for (i = r->r1->co.deg - 1; i >= 0; i--) {
			rp = (struct ec *) r->r1->co.adj[i];
			/* optmz: we can skip including in the conc. relation those
			 * reading or compound ecs. derived from the same condition
			 * r->c (since they will never be used to compute pe or extend
			 * the concurrency relation) */
			if (rp->c != r->c || EC_ISGEN (rp)) rp->m = m;
		}
	}
	ASSERT (m > 0);

	for (i = r->r2->co.deg - 1; i >= 0; i--) {
		rp = (struct ec *) r->r2->co.adj[i];
		if (rp->m == m) {
			al_add (&r->co, rp);
			al_add (&rp->co, r);
		}
	}
	u.unf.numco += r->co.deg * 2 - 1;
}

void co_add (struct ec *r)
{
	struct ec *rpp, *ri;
	struct event *e, *ep;
	struct cond *c;
	struct h *hp;
	struct ls *n;
	int i, nr;

	register int j;
	register int m1;
	register struct ec *rp;
	register int k;
	register int m;

	/* notation:
	 * r   is rho = (c, H)
	 * c   is r->c
	 * H   is r->h
	 * e   is r->h->e
	 * r_i are all ec. r_0, ..., r_{n-1} in r->h->ecl
	 * r'  is rho' = (c', H')
	 * c'  is rp->c
	 * H'  is rp->h
	 */

	/* 0. if r is a compound enriched condition, then _co_add_compound will
	 * deal with it, we only process here generating or reading enriched
	 * conditions :) */
	if (EC_ISCOMP (r)) return _co_add_compound (r);

	/* 1. every history already present in the prefix is *equal* or *less*
	 * than r->h.  Take care of ecs. whose history is equal to r->h */
	ASSERT (r->c);
	ASSERT (r->h);
	e = r->h->e;
	al_add (&r->co, r);

	for (i = e->post.deg - 1; i >= 0; i--) {
		c = e->post.adj[i];
		if (c == r->c) continue;
		for (n = c->ecl.next; n; n = n->next) {
			rp = ls_i (struct ec, n, nod);
			if (rp->h == r->h) {
				al_add (&r->co, rp);
				al_add (&rp->co, r);
			}
		}
	}

	for (i = e->cont.deg - 1; i >= 0; i--) {
		c = e->cont.adj[i];
		if (c == r->c) continue;
		for (n = c->ecl.next; n; n = n->next) {
			rp = ls_i (struct ec, n, nod);
			if (rp->h == r->h) {
				al_add (&r->co, rp);
				al_add (&rp->co, r);
			}
		}
	}

	/* 2. we now deal with histories H' smaller than H in \prec, if they
	 * exist */
	if (r->h->id == 0) {
		u.unf.numco += r->co.deg;
		return;
	}
	ASSERT (r->h->ecl.deg >= 1);

	/* 3. generate mark m */
	m = ++u.mark;
	ASSERT (m > 0);

	/* 5. mark with m pre(e) */
	/* 6. mark with m events e' reading from pre (e) */
	for (i = e->pre.deg - 1; i >= 0; i--) {
		c = (struct cond *) e->pre.adj[i];
		c->m = m;
		for (j = c->cont.deg - 1; j >= 0; j--) {
			((struct event *) c->cont.adj[j])->m = m;
		}
	}

	/* 4. now unmark those e' *in H* that read from pre (e) */
	for (i = r->h->sd.deg - 1; i >= 0; i--) {
		((struct event *) r->h->sd.adj[i])->m = 0;
	}

	/* 7. mark with m all enriched conditions r' in co(r_0) such that no
	 * event in H' reading some condition of cut(H') is marked with m and
	 * such that c' is not marked with m */
	/* optimz: keep account of the number of marked ecs. */
	ri = (struct ec *) r->h->ecl.adj[0];
	k = 0;
	ASSERT (ri);
	for (i = ri->co.deg - 1; i >= 0; i--) {
		rp = (struct ec *) ri->co.adj[i];
		if (rp->c->m == m) continue;

		for (rpp = rp; rpp; rpp = rpp->r2) {
			/* assert a correct structure of r' */
			ASSERT (rpp->h == 0 || (rpp->r1 == 0 && rpp->r2 == 0));
			ASSERT (rpp->r1 || rpp->h);
			ASSERT (rpp->r1 == 0 || rpp->r2);

			if (rpp->r1) hp = rpp->r1->h; else hp = rpp->h;
			ASSERT (hp);
			for (j = hp->rd.deg - 1; j >= 0; j--) {
				ep = (struct event *) hp->rd.adj[j];
				if (ep->m == m) break;
			}
			if (j >= 0) break;
		}
		if (! rpp) {
			rp->m = m; 
			k++;
		}
	}

	/* m = _co_add_inter (r, m, &nr); */

	/* 8. for every r_1, r_2, ..., r_{n-2} in r->h->ecl, and for every r'
	 * in co(r_i), mark r' with m only if r' is marked with (m-1) */
	for (i = 1; i < r->h->ecl.deg - 1; i++) {
		ri = (struct ec *) r->h->ecl.adj[i];

		/* generate a new mark */
		m1 = m;
		m = ++u.mark;
		ASSERT (m > 0);

		/* for all r' in co(r_i), mark if marked in the previous
		 * iteration */
		nr = k;
		for (j = ri->co.deg - 1; j >= 0 && k; j--) {
			rp = (struct ec *) ri->co.adj[j];
			if (rp->m == m1) {
				rp->m = m;
				k--;
			}
		}
		k = nr - k;
	}

	/* 9. all those events r' marked with m in co(r_{n-1}) are all the
	 * events in the current prefix of the unfolding concurrent to r */
	ri = (struct ec *) r->h->ecl.adj[r->h->ecl.deg - 1];
	for (i = ri->co.deg - 1; i >= 0 && k; i--) {
		rp = (struct ec *) ri->co.adj[i];
		if (rp->m == m) {
			al_add (&r->co, rp);
			al_add (&rp->co, r);
			k--;
		}
	}
	u.unf.numco += r->co.deg * 2 - 1;
}

int co_test (struct ec *r, struct ec *rp)
{
	ASSERT (al_test (&r->co, rp) == al_test (&rp->co, r));
	return r->co.deg < rp->co.deg ?
		al_test (&r->co, rp) : al_test (&rp->co, r);
}

