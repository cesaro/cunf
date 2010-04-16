
/*
 * Directed graph -- implementation
 */

#include "debug.h"
#include "glue.h"

#include "dg.h"

static void _dg_alloc (struct dg * n)
{
	/* 2 slots is the minimum size */
	if (n->deg == 0) {
		n->adj = gl_realloc (0, 2 * sizeof (struct dg *));
		return;
	}

	/* if current size is a power of two, duplicate the size */
	if ((n->deg & (n->deg - 1)) == 0) {
		n->adj = gl_realloc (n->adj, n->deg * 2 * sizeof (struct dg *));
	}

	/* otherwise, nothing to do! */
}

void dg_init (struct dg * n)
{
	ASSERT (n);
	n->deg = 0;
}

void dg_add (struct dg * n1, struct dg * n2)
{
	ASSERT (n1);
	ASSERT (n2);

	/* realloc (if necessary) the adj array and store the pointer there */
	_dg_alloc (n1);
	n1->adj[n1->deg] = n2;
	n1->deg++;
}

void dg_add2 (struct dg * n1, struct dg * n2)
{
	register int i;

	ASSERT (n1);
	ASSERT (n2);

	/* we skip the operation if the edge n1 -> n2 is already present in n1
	 */
	for (i = 0; i < n1->deg; i++) if (n1->adj[i] == n2) return;

	/* realloc (if necessary) the adj array and store the pointer there */
	_dg_alloc (n1);
	n1->adj[n1->deg] = n2;
	n1->deg++;
}

void dg_rem (struct dg * n1, struct dg * n2)
{
	/* search for the node n2 in the adjacency list of n1 and remove it
	 * from there */
	ASSERT (n1);
	ASSERT (n2);
	ASSERT (0);
}

int dg_test (const struct dg * n1, const struct dg * n2)
{
	register int i;

	for (i = 0; i < n1->deg; i++) if (n1->adj[i] == n2) return 1;
	return 0;
}

