
#include "netconv.h"
#include "marking.h"
#include "global.h"
#include "debug.h"
#include "pe.h"
#include "ls.h"
#include "dg.h"

#include "unfold.h"

static struct cond * _unfold_new_cond (struct event *e, struct place *p)
{
	struct cond *c;

	/* mallocate a new structure */
	c = gl_malloc (sizeof (struct cond));

	/* initialize structure's fields */
	ls_insert (&u.unf.conds, &c->nod);
	c->pre = e;
	dg_init (&c->post);
	dg_init (&c->cont);
	c->origin = p;
	c->m = 0;

	/* also the condition number */
	c->id = u.unf.numco++;

	/* finally, update the preset of c, the postset of e and append the
	 * condition to the list p->conds only if e is not a cutoff */
	dg_add (&e->post, &c->post);
	c->pre = e;
	if (! e->iscutoff) nl_push (&p->conds, c);

	return c;
}

static void _unfold_postset (struct event *e)
{
	struct trans *t;
	struct place *p;
	struct cond *c;
	int i;

	ASSERT (e);
	ASSERT (e->origin);

	/* if the output degree of the post node in e is the same as in the
	 * post node in e->origin, we are done :) */
	t = e->origin;
	if (e->post.deg == t->post.deg) return;

	/* otherwise, build a condition for each place in the original
	 * transition */
	for (i = t->post.deg - 1; i >= 0; i--) {
		p = dg_i (struct place, t->post.adj[i], post);
		_unfold_new_cond (e, p);
	}

	ASSERT (e->post.deg == t->post.deg);

	PRINT ("  Postset built for e%d:%s, ", e->id, e->origin->name);
	for (i = e->post.deg - 1; i >= 0; i--) {
		c = dg_i (struct cond, e->post.adj[i], post);
		PRINT (" c%d:%s", c->id, c->origin->name);
	}
	PRINT ("\n");
}

static void _unfold_init (void)
{
	struct event *e0;
	struct h *h0;

	/* allocate and initialize initial event */
	e0 = gl_malloc (sizeof (struct event));
	ls_insert (&u.unf.events, &e0->nod);
	dg_init (&e0->pre);
	dg_init (&e0->post);
	dg_init (&e0->cont);
	dg_init (&e0->ac);
	dg_init (&e0->hist);
	nl_push (&u.net.t0->events, e0);
	e0->origin = u.net.t0;
	e0->id = u.unf.numev++;
	e0->m = 0;
	e0->iscutoff = 0;
	ASSERT (e0->id == 0);
	PRINT ("+ Event e0 !!\n");

	/* e0 has only one history h0, consisting only on the event e0 */
	h0 = h_alloc (e0);
	ASSERT (h0->id == 0);

	/* insert the initial marking in the marking table */
	h_marking (h0);
	marking_add (h0);
	ASSERT (h0->corr == 0);

	/* build the postset of e0 */
	_unfold_postset (e0);

	/* set up the initial event in the unfolding */
	u.unf.e0 = e0;

	/* return the initial history */
}

void unfold (void)
{
	struct h *h0;
	struct h *h;

	/* 1. initialize structures (unfolding, mark. hash table and pe) */
	nc_create_unfolding ();
	marking_init ();
	pe_init ();
#ifdef CONFIG_MCMILLAN
	PRINT ("  Using McMillan order\n");
#else
	PRINT ("  Using Esparza-Romer-Vogler order\n");
#endif

	/* 2. insert the initial event e0, the initial history h0 and build the
	 * postset of e0 */
	/* 3. insert the initial marking in the marking hash table */
	_unfold_init ();

	/* 4. call pe_update with h0 to populate pe */
	ASSERT (u.unf.e0);
	ASSERT (u.unf.e0->hist.deg == 1);
	h0 = dg_i (struct h, u.unf.e0->hist.adj[0], nod);
	pe_update (h0);

	/* 5. loop, extract a history from pe */
	while (1) {

		h = pe_pop ();
		if (h == 0) break;

		/* 6. insert in the marking table its marking and determine if
		 * it is a cutoff (set field h->corr to the corresponding event
		 * or to null) */
		marking_add (h);
		PRINT ("\n- h%d/e%d:%s; size %d; is ",
				h->id,
				h->e->id,
				h->e->origin->name,
				h->size);
		if (h->corr != 0) {
			PRINT ("a cut-off! (corresponding h%d/e%d:%s)\n",
					h->corr->id,
					h->corr->e->id,
					h->corr->e->origin->name);
		} else {
			PRINT ("new!\n");
		}
		db_h (h);

		/* 7. build the postset of the maximal transition, if not
		 * already present in the unfolding */
		if (h->corr == 0) h->e->iscutoff = 0;
		_unfold_postset (h->e);

		/* 8. update pe with this new history only if it is not a
		 * cutoff */
		if (h->corr == 0) pe_update (h);
	}
}

