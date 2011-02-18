
/*
 * Static single-linked list with tail -- implementation
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
#include "lst/lst.h"

#ifdef CONFIG_DEBUG
static void _lst_assert (struct lst * l)
{
	ASSERT (l);
	ASSERT (l->head || l->tail == 0);
	ASSERT (l->head == 0 || l->tail);

	/* check for no loops */
}
#else
#define _lst_assert(l)
#endif

void lst_init (struct lst * l)
{
	l->head = 0;
	l->tail = 0;
}

inline void lst_insert (struct lst * l, struct lstn * n)
{
	/* insert the node n in the beginning of the list l */
	ASSERT (l);
	ASSERT (n);

	n->next = l->head;
	l->head = n;
	if (l->tail == 0) l->tail = n;
	_lst_assert (l);
}

void lst_append (struct lst * l, struct lstn * n)
{
	/* append the node n to the end of the list l */
	ASSERT (l);
	ASSERT (n);

	if (l->head == 0) {
		l->head = n;
	} else {
		l->tail->next = n;
	}

	n->next = 0;
	l->tail = n;
	_lst_assert (l);
}

void lst_remove (struct lst * l, struct lstn * p)
{
	/* remove from the list l the next node of node p */
	/* FIXME -- assert that p is in the list */

	ASSERT (l->head);
	if (l->head == 0) return;
	
	/* we remove the head of the list if p is null */
	if (p == 0) {
		if (l->head == l->tail) l->tail = 0;
		l->head = l->head->next;
		return;
	}

	/* otherwise, we skip next node of node p */
	ASSERT (p->next);
	if (p->next == 0) return;
	if (l->tail == p->next) l->tail = p;
	p->next = p->next->next;
	_lst_assert (l);
}

void lst_shift (struct lst * l)
{
	/* move the first element of the list to the tail */
	ASSERT (l);

	/* nothing to do if there is only zero or one elements */
	if (l->head == 0 || l->head == l->tail) return;

	l->tail->next = l->head;
	l->tail = l->head;
	l->head = l->head->next;
	l->tail->next = 0;
	_lst_assert (l);
} 
void lst_push (struct lst * l, struct lstn * n)
{
	/* same as ls_insert */
	ASSERT (l);
	ASSERT (n);

	n->next = l->head;
	l->head = n;
	if (l->tail == 0) l->tail = n;
	_lst_assert (l);
}

struct lstn * lst_pop (struct lst * l)
{
	struct lstn * n;

	/* extract and return the head of the list l, or null in case l is
	 * empty */
	ASSERT (l);

	if (l->head == 0) return 0;
	if (l->head == l->tail) l->tail = 0;
	n = l->head;
	l->head = l->head->next;
	_lst_assert (l);
	return n;
}

int lst_iter (struct lst * l, int (* callback) (struct lstn * n))
{
	register struct lstn * n;
	int ret;

	for (n = l->head; n; n = n->next) {
		ret = callback (n);
		if (ret < 0) return ret;
	}
	return 0;
}

/*
List 00112233 "Hi world"
 0000 00112233;   0000 00112233;   0000 00112233;   0000 00112233
 0000 00112233;   0000 00112233;   0000 00112233;   0000 00112233
 0000 00112233;   0000 00112233;   0000 00112233;   0000 00112233
 0000 00112233;   0000 00112233;   0000 00112233;   0000 00112233
*/
void lst_print (struct lst * l, const char * heading)
{
	struct lstn * n;
	int i;

	ASSERT (l);

	PRINT ("List %08lx ", (unsigned long) l);
	if (heading) { PRINT (" \"%s\"", heading); }

	for (n = l->head, i = 0; n; n = n->next, i++) {
		if (i % 4 == 0) { PRINT ("\n"); }
		PRINT (" %04x %08lx;  ", i, (unsigned long) n);
	}
	PRINT ("\n");
}

