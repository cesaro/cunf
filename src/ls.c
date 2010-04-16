
/*
 * Static single-linked list -- implementation
 */

#include "ls.h"
#include "debug.h"
#include "config.h"

#ifdef CONFIG_DEBUG
static void _ls_assert (struct ls * l)
{
	ASSERT (l);
	/* check for no loops */
}
#else
#define _ls_assert(l)
#endif

void ls_init (struct ls * l)
{
	l->next = 0;
}

inline void ls_insert (struct ls * l, struct ls * n)
{
	/* insert the node n in the beginning of the list l */
	ASSERT (l);
	ASSERT (n);

	n->next = l->next;
	l->next = n;
	_ls_assert (l);
}

void ls_append (struct ls * l, struct ls * n)
{
	register struct ls *tail;

	/* append the node n to the end of the list l */
	ASSERT (l);
	ASSERT (n);

	/* special case: the queue is empty */
	n->next = 0;
	if (l->next == 0) {
		l->next = n;
		return;
	}

	/* common case, the queue is not empty */
	for (tail = l->next; tail->next; tail = tail->next);
	tail->next = n;
	_ls_assert (l);
}

void ls_remove (struct ls * l, struct ls * p)
{
	/* remove from the list l the node n such that n is the next node of
	 * node p */

	ASSERT (l->next);
	if (l->next == 0) return;
	
	/* we remove the head of the list if p is null */
	if (p == 0) {
		l->next = l->next->next;
		return;
	}

	/* otherwise, we skip next node of node p */
	ASSERT (p->next);
	if (p->next == 0) return;
	p->next = p->next->next;
	_ls_assert (l);
}

void ls_shift (struct ls * l)
{
	struct ls * tail;
	
	/* move the first element of the list to the last position */
	ASSERT (l);

	/* nothing to do if there is only zero or one elements */
	if (l->next == 0 || l->next->next == 0) return;

	/* search for the tail and perform the update */
	for (tail = l->next; tail->next; tail = tail->next);
	tail->next = l->next;
	l->next = l->next->next;
	tail->next = 0;
	_ls_assert (l);
}

void ls_push (struct ls * l, struct ls * n)
{
	/* insert the node n in the head of the list l (same as ls_insert) */
	ASSERT (l);
	ASSERT (n);

	n->next = l->next;
	l->next = n;
	_ls_assert (l);
}

struct ls * ls_pop (struct ls * l)
{
	struct ls * n;

	/* extract and return the head of the list l, or null in case l is
	 * empty */
	ASSERT (l);

	if (l->next == 0) return 0;
	n = l->next;
	l->next = l->next->next;
	_ls_assert (l);
	return n;
}

int ls_iter (struct ls * l, int (* callback) (struct ls * n))
{
	register struct ls * n;
	int ret;

	for (n = l->next; n; n = n->next) {
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
void ls_print (struct ls * l, const char * heading)
{
	struct ls * n;
	int i;

	ASSERT (l);	

	PRINT ("List %08lx ", (unsigned long) l);
	if (heading) PRINT (" \"%s\"", heading);

	for (n = l->next, i = 0; n; n = n->next, i++) {
		if (i % 4 == 0) PRINT ("\n");
		PRINT (" %04x %08lx;  ", i, (unsigned long) n);
	}
	PRINT ("\n");
}

