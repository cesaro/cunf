
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

void marking_init (void)
{
	int i;

	/* initialize the hash table; h.size is the size of the array h.tab;
	 * h.tab is a vector of lists; h.tab[i] is a singly-linked list storing
	 * all the pairs (marking,history) with the same marking */
	
	hash.size = 1 + u.net.numpl * 4;
	hash.tab = gl_malloc (hash.size * sizeof (struct ls));
	for (i = hash.size - 1; i >= 0; i--) ls_init (hash.tab + i);
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

void marking_add (struct h *h)
{
	struct hash_entry *he, *nhe;
	struct ls *buckl, *n;
	int ret;

	ASSERT (h);
	ASSERT (h->marking);
	ASSERT (h->corr != 0); /* histories always initialized as cutoffs */

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

#if CONFIG_MCMILLAN
	/* if it is the case, and the corresponding history is smaller,
	 * according to the McMillan's order, then it is a cutoff */
	if (n && h_cmp (he->h, h) < 0) {
#else
	/* if present in the hash table, then we know h is a cutoff, as we are
	 * sure that the corresponding history is smaller (the ERV order is
	 * total) */
	if (n) {
		ASSERT (h_cmp (he->h, h) < 0);
#endif

		/* deallocate the marking and set history h as a cutoff, whose
		 * corresponding history is he->h */
		ASSERT (he);
		nl_delete (h->marking);
		h->marking = 0;
		h->corr = he->h;
		PRINT ("  h%d/e%d:%s is cutoff of h%d/e%d:%s\n",
				h->id,
				h->e->id,
				h->e->origin->name,
				he->h->id,
				he->h->e->id,
				he->h->e->origin->name);
		return;
	}

	/* otherwise, insert h into the table */
	h->corr = 0;
	nhe = gl_malloc (sizeof (struct hash_entry));
	nhe->h = h;
	ls_insert (buckl, &nhe->nod);
	return;
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

