
// #include <stdio.h>

#include "nodelist.h"
#include "marking.h"
#include "global.h"
#include "debug.h"
#include "glue.h"
#include "ls.h"
#include "h.h"

struct hash_entry {
	struct ls nod;
	struct h *h;
};

struct hash {
	struct ls *tab;
	int size;
};

struct hash hash;
struct nl *cutoffl;
struct nl *corrl;

static int _marking_hash (const struct nl *l)
{
	unsigned int val, i;

	/* compute the hash value of a marking */
	val = 0;
	i = 1;
	for (; l; l = l->next) {
		val += ((const struct place *) (l->node))->id * i;
		i++;
	}
	return val % hash.size;
}

static void _marking_cutoff (struct h *cutoffh, struct h *corrh)
{
	/* cutoffh is a cutoff of the history corrh */
	nl_push (&cutoffl, cutoffh);
	nl_push (&corrl, corrh);
}

void marking_init (void)
{
	int i;

	/* initialize the hash table; h.size is the size of the array h.tab;
	 * h.tab is a vector of lists; h.tab[i] is a singly-linked list storing
	 * all the pairs (marking,history) with the same marking */
	
	hash.size = 1 + u.net.numpl * 4;
	hash.tab = gl_malloc (hash.size * sizeof (struct ls));
	for (i = hash.size - 1; i >= 0; i--) ls_init (hash.tab + i);

	cutoffl = 0;
	corrl = 0;
}

int marking_find (const struct h *h)
{
	struct hash_entry *he;
	struct ls *n;
	int ret;

	/* if the marking h->marking is in the hash table, it is somewere in
	 * the list h.tab[hash(h->marking)]; return the logical condition of
	 * h->marking to be in the table
	 */

	n = hash.tab + _marking_hash (h->marking);
	for (n = n->next; n; n = n->next) {
		he = ls_i (struct hash_entry, n, nod);
		ret = nl_compare (he->h->marking, h->marking);
		if (ret == 0) return 1;
	}

	return 0;
}

int marking_add (struct h *h)
{
	struct hash_entry *he, *nhe;
	struct ls *buckl, *n;
	int ret;

	ASSERT (h);
	ASSERT (h->marking);

	/* add the marking h->marking to the hash table if there is no
	 * other history h' such that h->marking = h'->marking; if such h'
	 * exists, then h is a cutoff; the function returns 1 iff h is a cutoff
	 */

	/* determine if the marking h->marking is in the hash table */
	buckl = hash.tab + _marking_hash (h->marking);
	for (n = buckl->next; n; n = n->next) {
		he = ls_i (struct hash_entry, n, nod);
		ret = nl_compare (he->h->marking, h->marking);
		if (ret == 0) break;
	}

	/* if it is the case, deallocate the marking and set history h as a
	 * cutoff, whose corresponding history is he->h */
	if (n) {
		ASSERT (he);
		nl_delete (h->marking); /* FIXME -- should this be done here ? */
		h->marking = 0;
		_marking_cutoff (h, he->h);
		return 1;
	}

	/* otherwise, insert h into the table */
	nhe = gl_malloc (sizeof (struct hash_entry));
	nhe->h = h;
	ls_insert (buckl, &nhe->nod);
	return 0;
}

static void _marking_print (const struct nl *l)
{
	if (! l) return;
	_marking_print (l->next);

	PRINT (" %s", ((const struct place*) (l->node))->name);
}

void marking_print (const struct h *h)
{
	_marking_print (h->marking);
}

