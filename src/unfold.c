
#include "netconv.h"
#include "marking.h"
#include "global.h"
#include "ls/ls.h"
#include "al/al.h"
#include "debug.h"
#include "glue.h"
#include "pe.h"
#include "co.h"

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
	c->id = u.unf.numco++;

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

static void _unfold_combine (struct ec *r)
{
	struct ec *rpp;
	struct ec *rp;
	struct ls *n;

	ASSERT (r);
	ASSERT (r->c);

	for (n = r->c->ecl.next; n; n = n->next) {
		rp = ls_i (struct ec, n, nod);
		if (EC_ISGEN (rp) || r == rp) continue;
		if (co_test (r, rp)) {
			rpp = ec_alloc2 (r, rp);
			co_add (rpp);
			pe_update_read (rpp);
		}
	}
}

static void _unfold_enriched (struct h *h)
{
	struct event *e;
	struct trans *t;
	struct cond *c;
	struct ec *r;
	int i;

	ASSERT (h);
	ASSERT (h->corr == 0);
	ASSERT (h->e);
	ASSERT (h->e->ft);
	ASSERT (h->e->iscutoff == 0);
	ASSERT (h->e->post.deg == h->e->ft->post.deg);

	/* append a new enriched condition r for each c in post(e), compute the
	 * concurrency relation for r and use r to update pe with new possible
	 * extensions */
	e = h->e;
	t = e->ft;
	for (i = e->post.deg - 1; i >= 0; i--) {
		c = (struct cond *) e->post.adj[i];
		r = ec_alloc (c, h);
		co_add (r);
		pe_update_gen (r);
	}

	/* then, do the same with cont(e) :) */
	for (i = e->cont.deg - 1; i >= 0; i--) {
		c = (struct cond *) e->cont.adj[i];
		if (c->fp->post.deg == 0) continue;
		r = ec_alloc (c, h);
		co_add (r);
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
		DPRINT ("  At size %6d, %d histories\n", h->size, i);
	}
}

static int __compare (int * events, int nr, struct dls *l) {
	int len, i;
	struct h *h;

	len = 0;
	for (l = l->next; l; l = l->next) {
		len++;
		for (i = nr - 1; i >= 0; i--) {
			h = dls_i (struct h, l, auxnod);
			if (events[i] == h->e->id) break;
		}
		if (i < 0) return 0;
	}
	if (len != nr) return 0;

	/* found! */
	return 1;
}

#if 0
	int eid = 112;
	int nr = 67;
	int events[] = {54, 23, 2, 61, 53, 109, 46, 72, 40, 77, 41, 55, 3, 52,
	110, 1, 59, 103, 36, 82, 19, 60, 18, 44, 74, 20, 29, 100, 9, 12, 58, 8,
	62, 57, 76, 104, 50, 56, 35, 31, 6, 63, 39, 21, 4, 112, 67, 32, 80, 16,
	107, 24, 13, 48, 10, 14, 28, 5, 15, 7, 17, 11, 66, 65, 69, 45, 0};

	h21
	int eid = 20;
	int nr = 18;
	int events[] = {20, 0, 1, 12, 13, 14, 15, 16, 17, 19, 2, 3, 4, 5, 6, 7, 8, 9};

	h127
	int eid = 56;
	int nr = 36;
	int events[] = {0, 1, 32, 35, 36, 4, 40, 10, 11, 12, 13, 14, 2, 20, 21,
	23, 24, 29, 3, 31, 41, 46, 15, 16, 17, 18, 19, 5, 50, 52, 55, 56, 6, 7,
	8, 9};

#endif

void __test (void) {
	struct event *e;
	struct ls *n;
	struct h *h;
	struct dls l;
	int i;

	int eid = 93;
	int nr = 38;
	int events[] = {2, 6, 4, 3, 1, 59, 27, 204, 245, 264, 78, 16, 93, 26,
	20, 91, 13, 10, 9, 12, 28, 5, 198, 47, 226, 15, 169, 123, 8, 111, 165,
	7, 11, 143, 66, 31, 22, 0};

	e = 0;
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == eid) break;
	}
	ASSERT (n && e->id == eid);

	for (i = e->hist.deg - 1; i >= 0; i--) {
		h = (struct h *) e->hist.adj[i];
		h_list (&l, h);
		if (__compare (events, nr, &l)) break;
	}

	if (i >= 0) {
		DPRINT ("found! h%d\n", h->id);
		BREAK (1);
	} else {
		DPRINT ("not found :(\n");
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

	pe_term ();
	/* __test (); */
	/* db_h2dot (); */
}


