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
#include "ls/ls.h"
#include "glue.h"

#include "netconv.h"

/****************************************************************************/
/* nc_create_net							    */
/* Creates a new net without places or transitions.			    */

void nc_create_net()
{
	ls_init (&u.net.places);
	ls_init (&u.net.trans);
	u.net.numpl = u.net.numtr = 0;
	u.net.t0 = 0;
	u.net.isplain = 1;
}

void nc_create_unfolding()
{
	ls_init (&u.unf.conds);
	ls_init (&u.unf.events);
	u.unf.numco = u.unf.numev = u.unf.numh = u.unf.numduph = 0;
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

	p->id = u.net.numpl++;
	al_init (&p->pre);
	al_init (&p->post);
	al_init (&p->cont);
	ls_init (&p->conds);
	p->m = 0;

	return p;
}

struct trans * nc_create_transition (void)
{
	struct trans * t;

	t = gl_malloc (sizeof (struct trans));
	ls_insert (&u.net.trans, &t->nod);

	t->id = ++u.net.numtr;
	al_init (&t->pre);
	al_init (&t->post);
	al_init (&t->cont);
	ls_init (&t->events);
	t->m = 0;
	t->parikhcnt1 = 0;
	t->parikhcnt2 = 0;

	return t;
}

/****************************************************************************/
/* nc_create_arc							    */
/* Create an arc between two nodes (place->transition or transition->place) */

int nc_create_arc (struct al * src_post, struct al * dst_pre,
		void * src, void * dst)
{
	if (al_test (src_post, dst)) return 0;
	al_add (src_post, dst);
	al_add (dst_pre, src);
	return 1;
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

