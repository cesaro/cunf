/****************************************************************************/
/* netconv.c                                                                */
/*                                                                          */
/* Functions for creating and modifying Petri net elements		    */
/****************************************************************************/

#include <string.h>

#include "nodelist.h"
#include "unfold.h"
#include "global.h"
#include "debug.h"
#include "glue.h"
#include "ls.h"

#include "netconv.h"

/****************************************************************************/
/* nc_create_net							    */
/* Creates a new net without places or transitions.			    */

void nc_create_net()
{
	ls_init (&u.net.places);
	ls_init (&u.net.trans);
	u.net.numpl = u.net.numtr = 0;
}

void nc_create_unfolding()
{
	ls_init (&u.unf.conds);
	ls_init (&u.unf.events);
	u.unf.numco = u.unf.numev = u.unf.numh = 0;
	u.unf.e0 = 0;
}

/****************************************************************************/
/* nc_create_{place,transition}						    */
/* Creates a new place or transition in the given net. The new node has no  */
/* incoming or outgoing arcs and is unmarked.				    */

struct place * nc_create_place (void)
{
	struct place * p;
	
	p = gl_malloc (sizeof (struct place));
	ls_insert (&u.net.places, &p->nod);

	dg_init (&p->pre);
	dg_init (&p->post);
	dg_init (&p->cont);
	p->conds = 0;
	p->id = u.net.numpl++;

	return p;
}

struct trans * nc_create_transition (void)
{
	struct trans * t;

	t = gl_malloc (sizeof (struct trans));
	ls_insert (&u.net.trans, &t->nod);

	dg_init (&t->pre);
	dg_init (&t->post);
	dg_init (&t->cont);
	t->id = u.net.numtr++;

	return t;
}

/****************************************************************************/
/* nc_create_arc							    */
/* Create an arc between two nodes (place->transition or transition->place) */

void nc_create_arc (struct dg * src_post, struct dg * dst_pre,
		struct dg * src_pre, struct dg * dst_post)
{
	dg_add2 (src_post, dst_post);
	dg_add2 (dst_pre, src_pre);
}

/****************************************************************************/
/* nc_compute_sizes							    */
/* compute (maximal) sizes of transition presets/postsets		    */

void nc_compute_sizes (void)
{
	struct ls *n;
	struct trans *t;

	u.net.maxpre = u.net.maxpost = u.net.maxcont = 0;
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);

		if (u.net.maxpre < t->pre.deg) u.net.maxpre = t->pre.deg;
		if (u.net.maxpost < t->post.deg) u.net.maxpost = t->post.deg;
		if (u.net.maxcont < t->cont.deg) u.net.maxcont = t->cont.deg;
	}
}

/*****************************************************************************/

void nc_static_checks (const char * stoptr)
{
	struct trans *t;
	struct ls *n;

	ASSERT (u.stoptr == 0);
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);
		if (t->pre.deg == 0 && t != u.net.t0) {
			gl_warn ("%s is not restricted", t->name);
		}
		if (stoptr && !strcmp (t->name, stoptr)) {
			u.stoptr = t;
		}
	}

	if (stoptr && u.stoptr == 0) {
		gl_err ("Transition '%s' not found", stoptr);
	}

	ASSERT (u.net.t0);
	if (u.net.t0->post.deg == 0) gl_err ("No initial marking!");
}

