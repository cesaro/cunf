/****************************************************************************/
/* netconv.c                                                                */
/*                                                                          */
/* Functions for creating and modifying Petri net elements		    */
/****************************************************************************/

#include <string.h>

#include "global.h"
#include "nodelist.h"
#include "unfold.h"
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
	u.unf.numco = u.unf.numev = 0;
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

	p->pre = p->post = p->conds = NULL;
	p->num = ++u.net.numpl;
	return p;
}

struct trans * nc_create_transition (void)
{
	struct trans * t;

	t = gl_malloc (sizeof (struct trans));
	ls_insert (&u.net.trans, &t->nod);

	t->pre = t->post = NULL;
	t->pre_size = 0;
	t->num = ++u.net.numtr;
	return t;
}

/****************************************************************************/
/* nc_create_arc							    */
/* Create an arc between two nodes (place->transition or transition->place) */

void nc_create_arc (struct nl **fromlist, struct nl **tolist,
		   void *from, void *to)
{
	struct nl *list;

	for (list = *fromlist; list; list = list->next)
		if (list->node == to) return;

	nl_push(fromlist,to);
	nl_push(tolist,from);
}

/****************************************************************************/
/* nc_compute_sizes							    */
/* compute (maximal) sizes of transition presets/postsets		    */

void nc_compute_sizes (void)
{
	struct ls *n;
	struct trans *t;
	struct nl *nl;
	int k;

	u.net.maxpre = u.net.maxpost = u.net.maxcont = 0;
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_item (struct trans, n, nod);

		k = 0;
		for (nl = t->pre; nl; nl = nl->next) k++;
		t->pre_size = k;
		if (u.net.maxpre < k) u.net.maxpre = k;

		k = 0;
		for (nl = t->post; nl; nl = nl->next) k++;
		t->post_size = k;
		if (u.net.maxpost < k) u.net.maxpost = k;

		k = 0;
		for (nl = t->cont; nl; nl = nl->next) k++;
		t->cont_size = k;
		if (u.net.maxcont < k) u.net.maxcont = k;
	}
}

/*****************************************************************************/

void nc_static_checks (void)
{
	struct place *p;
	struct trans *t;
	struct ls *n;
	int found;

	found = 0;
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_item (struct trans, n, nod);
		if (t->pre == 0 || t->post == 0) {
			gl_warn ("%s is not restricted", t->name);
		}
		if (u.stoptr && !strcmp (t->name, u.stoptr)) {
			found = 1;
		}
	}

	if (u.stoptr && ! found) {
		gl_err ("Transition '%s' not found", u.stoptr);
	}

	p = 0;
	for (n = u.net.places.next; n; n = n->next) {
		p = ls_item (struct place, n, nod);
		if (p->marked) break;
	}
	if (! p) gl_err ("No initial marking!");
}

