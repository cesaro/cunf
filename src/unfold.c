
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

#include "netconv.h"
#include "marking.h"
#include "global.h"
#include "ls/ls.h"
#include "al/al.h"
#include "da/da.h"
#include "debug.h"
#include "glue.h"
#include "pe.h"
#include "ec.h"

#include "unfold.h"

static struct cond * _unfold_new_cond (struct event *e, struct place *p)
{
	struct cond *c;

	/* mallocate a new structure */
	c = gl_malloc (sizeof (struct cond));

	/* initialize structure's fields */
	ls_insert (&u.unf.conds, &c->nod);
	c->pre = e;
	al_init (&c->post);
	al_init (&c->cont);
	c->fp = p;
	ls_insert (&p->conds, &c->pnod);
	ls_init (&c->ecl);
	c->m = 0;

	/* also the condition number */
	c->id = u.unf.numcond++;

	/* finally, update the preset of c, the postset of e and append the
	 * condition to the list p->conds only if e is not a cutoff */
	c->pre = e;
	al_add (&e->post, c);
	return c;
}

static void _unfold_postset (struct event *e)
{
	struct trans *t;
	struct place *p;
	int i;

	ASSERT (e);
	ASSERT (e->ft);

	/* if the output degree of the post node in e is the same as in the
	 * post node in e->origin, postset conditions are already unfolded;
	 * otherwise we create a new condition for each place */

	t = e->ft;
	if (e->post.deg == t->post.deg) return;

	for (i = t->post.deg - 1; i >= 0; i--) {
		p = (struct place *) t->post.adj[i];
		_unfold_new_cond (e, p);
	}
}

static void _unfold_combine (register struct ec *r)
{
	register struct ec *rp;
	register int i, j;
	static struct da l;
	static int init = 1;

	ASSERT (r);
	ASSERT (r->c);

	/* compute those rp from rco(r) which must be combined with r */
	if (init) {
		init = 0;
		da_init (&l, struct ec *);
	}
	j = 0;
	for (i = r->rco.deg - 1; i >= 0; i--) {
		rp = (struct ec *) r->rco.adj[i];
		if (! ec_included (r, rp)) da_push (&l, j, rp, struct ec *);
	}

	for (j--; j >= 0; j--) {
		u.unf.numcomp++;
		rp = ec_alloc2 (r, da_i (&l, j, struct ec *));
		ec_conc (rp);
		pe_update_read (rp);
	}

	/* !!! */
	al_term (&r->rco);
}

static void _unfold_enriched (struct h *h)
{
	struct event *e;
	struct cond *c;
	struct ec *r;
	int i;

	ASSERT (h);
	ASSERT (h->corr == 0);
	ASSERT (h->e);
	ASSERT (h->e->ft);
	ASSERT (h->e->iscutoff == 0);
	ASSERT (h->e->post.deg == h->e->ft->post.deg);

	/* if the prefix must not go beyond some depth and h already has that
	 * depth, then no history generated using h need to be inserted */
	if (u.depth && h->depth >= u.depth) return;

	/* append a new enriched condition r for each c in post(e), compute the
	 * concurrency relation for r and use r to update pe with new possible
	 * extensions */
	e = h->e;
	for (i = e->post.deg - 1; i >= 0; i--) {
		c = (struct cond *) e->post.adj[i];
		if (c->fp->post.deg + c->fp->cont.deg == 0) continue;

		r = ec_alloc (c, h);
		u.unf.numgen++;
		ec_conc (r);
		pe_update_gen (r);
	}

	/* then, do the same with cont(e) :) */
	for (i = e->cont.deg - 1; i >= 0; i--) {
		c = (struct cond *) e->cont.adj[i];
		if (c->fp->post.deg == 0) continue;
		r = ec_alloc (c, h);
		u.unf.numread++;
		ec_conc (r);
		pe_update_read (r);
		_unfold_combine (r);
	}
}

static void _unfold_init (void)
{
	struct event *e0;
	struct h *h0;

	/* allocate and initialize initial event */
	e0 = gl_malloc (sizeof (struct event));
	ls_insert (&u.unf.events, &e0->nod);
	al_init (&e0->pre);
	al_init (&e0->post);
	al_init (&e0->cont);
	al_init (&e0->ac);
	al_init (&e0->hist);
	e0->ft = u.net.t0;
	ls_insert (&u.net.t0->events, &e0->tnod);
	e0->id = u.unf.numev++;
	e0->iscutoff = 0;
	e0->m = 0;
	ASSERT (e0->id == 0);
	DPRINT ("+ Event e0 !!\n");

	/* e0 has only one history h0, consisting only on the event e0 */
	h0 = h_alloc (e0);
	ASSERT (h0->id == 0);

	/* insert the initial marking in the marking table */
	h_marking (h0);
	marking_add (h0);
	ASSERT (h0->corr == 0);

	/* set up the initial event in the unfolding */
	u.unf.e0 = e0;

	_unfold_postset (e0);
	_unfold_enriched (h0);
}

static void _unfold_progress (struct h *h)
{
	static int i = 0;
	i++;

	DPRINT ("\n- h%d/e%d:%s; size %d; is ",
			h->id,
			h->e->id,
			h->e->ft->name,
			h->size);
	if (h->corr != 0) {
		DPRINT ("a cut-off! (corr. h%d/e%d:%s)\n",
				h->corr->id,
				h->corr->e->id,
				h->corr->e->ft->name);
	} else {
		DPRINT ("new!\n");
	}

	if ((i & 0xfff) == 0) {
		PRINT ("  At size %6d, %d histories\n", h->size, i);
	}
}

void unfold (void)
{
	struct h *h;

#ifdef CONFIG_MCMILLAN
	DPRINT ("  Using McMillan order\n");
#else
#ifdef CONFIG_ERV
	DPRINT ("  Using Esparza-Romer-Vogler order\n");
#else
	DPRINT ("  Using Mole ERV order\n");
#endif
#endif

	nc_create_unfolding ();
	marking_init ();
	pe_init ();
	h_init ();
	_unfold_init ();

	while (1) {
		h = pe_pop ();
		if (h == 0) break;

		marking_add (h);
		_unfold_progress (h);
		_unfold_postset (h->e);

		if (h->corr == 0) {
			h->e->iscutoff = 0;
			_unfold_enriched (h);
		}
	}

	h_term ();
	pe_term ();
}

