
/*
 * Static doubly-linked list
 * 
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

#include "glue.h"
#include "dls/dls.h"

#ifdef CONFIG_DEBUG
#undef _SLOW_CHECK
static void _dls_assert (struct dls * l)
{
#ifdef _SLOW_CHECK
	register struct dls * n, * p;
	int i;
#endif

	/* empty list */
	if (l->next == 0) {
		ASSERT (l->prev == 0);

	} else {
		/* not empty list, check the null pointers in the head and tail
		 * elements */
		ASSERT (l->prev != 0);
		ASSERT (l->next->prev == 0);
		ASSERT (l->prev->next == 0);
	}

#ifdef _SLOW_CHECK
	/* iterate over the elements */
	n = l->next;
	p = 0;
	for (n = l->next; n; n = n->next) {
		ASSERT (n->prev == p);
		p = n;
	}
	ASSERT (l->prev == p);
#endif
}
#else 
#define _dls_assert(l)
#endif

void dls_init (struct dls * l)
{
	l->next = 0;
	l->prev = 0;
}

void dls_insert (struct dls * l, struct dls * n)
{
	/* insert the node n in the beginning of the list l */

	/* special case: the queue is empty */
	if (l->next == 0) {
		n->next = n->prev = 0;
		l->next = l->prev = n;

	/* common case: the queue is not empty */
	} else {
		n->prev = 0;
		n->next = l->next;
		n->next->prev = n;
		l->next = n;
	}

	/* assert a correct state of the list */
	_dls_assert (l);
	ASSERT (l->next == n);
}

void dls_append (struct dls * l, struct dls * n)
{
	/* append the node n to the end of the list l */

	/* special case: the queue is empty */
	if (l->next == 0) {
		n->next = n->prev = 0;
		l->next = l->prev = n;

	/* common case: the queue is not empty */
	} else {
		ASSERT (l->next != n);
		ASSERT (l->prev != n);
		n->next = 0;
		n->prev = l->prev;
		n->prev->next = n;
		l->prev = n;
	}

	/* assert a correct state of the list */
	_dls_assert (l);
	ASSERT (l->prev == n);
}

void dls_remove (struct dls * l, struct dls * n)
{
	/* remove the node n from the list l */

	/* assert a correct state of the list */
	ASSERT (l->next);
	ASSERT (l->prev);

	/* the only item in the list is the node n */
	if (l->next == n && l->prev == n) {
		l->next = 0;
		l->prev = 0;

	/* the node is the first and not the last */
	} else if (l->next == n) {
		l->next = n->next;
		n->next->prev = 0;

	/* the node is the last and not the first */
	} else if (l->prev == n) {
		l->prev = n->prev;
		n->prev->next = 0;

	/* the node is in-between the list */
	} else {
		n->prev->next = n->next;
		n->next->prev = n->prev;
	}

	/* assert a correct state of the list */
	_dls_assert (l);
}

void dls_shift (struct dls * l)
{
	/* move the first element of the list to the last position */

	struct dls * n;

	/* assert a correct state of the list */
	ASSERT (l->next);
	ASSERT (l->prev);

	/* nothing to do if there is only one element !! */
	if (l->next == l->prev) return;

	/* do it */
	n = l->next;
	l->next = l->next->next;
	l->next->prev = 0;
	l->prev->next = n;
	n->prev = l->prev;
	n->next = 0;
	l->prev = n;

	/* assert a correct state of the list */
	_dls_assert (l);
}

int dls_iter (struct dls * l, int (* callback) (struct dls * n))
{
	register struct dls * n;
	int ret;

	for (n = l->next; n; n = n->next) {
		ret = callback (n);
		if (ret < 0) return ret;
	}
	return 0;
}

void dls_print (struct dls * l, const char * heading)
{
	struct dls * n;
	int i;

	ASSERT (l);	

	PRINT ("List %08lx ", (unsigned long) l);
	if (heading) { PRINT (" \"%s\"", heading); }

	for (n = l->next, i = 0; n; n = n->next, i++) {
		if (i % 4 == 0) { PRINT ("\n"); }
		PRINT (" %04x %08lx;  ", i, (unsigned long) n);
	}
	PRINT ("\n");
}

void dls_push (struct dls * l, struct dls * n)
{
	/* insert the node n in the beginning of the list l */

	n->prev = 0;
	n->next = l->next;
	if (l->next == 0) {
		ASSERT (l->prev == 0);
		l->prev = n;
	} else {
		l->next->prev = n;
	}
	l->next = n;	

	/* and assert a correct state of the list */
	_dls_assert (l);
	ASSERT (l->next == n);
}

struct dls * dls_pop (struct dls * l)
{
	struct dls * n;

	ASSERT (l->next);
	ASSERT (l->prev);

	n = l->next;
	if (l->prev == l->next) {
		l->prev = l->next = 0;
	} else {
		l->next = l->next->next;
		l->next->prev = 0;
	}

	/* assert a correct state of the list */
	ASSERT (l->next != n);
	_dls_assert (l);

	return n;
}

