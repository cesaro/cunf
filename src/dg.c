
/*
 * Directed graph -- implementation
 */

#include <string.h>
#include <limits.h>
#include <stdint.h>

#include "debug.h"
#include "glue.h"

#include "dg.h"

static uint32_t _dg_nextpow (uint32_t i)
{
	uint32_t m;
	int a;

	/* next power to 2^31 is 2^32, or 0 mod 2^32 :) */
	if (i & (1 << 31)) return 0;

	/* shift a bits to the right if m >= i, otherwise do it to the left;
	 * and stop when a is zero */
	m = 1 << 16;
	for (a = 8; a; a /= 2) if (m >= i) m >>= a; else m <<= a;
	return m;
}

static void _dg_alloc (struct dg * n)
{
	/* if current size is a power of two */
	if ((n->deg & (n->deg - 1)) == 0) {

		/* 2 slots is the minimum size */
		if (n->deg == 0) {
			n->adj = gl_realloc (0, 2 * sizeof (struct dg *));
			return;
		}

		/* duplicate the size */
		n->adj = gl_realloc (n->adj, n->deg * 2 * sizeof (struct dg *));
	}
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

void dg_cpy (struct dg * dst, struct dg * src)
{
	int size;

	ASSERT (dst);
	ASSERT (src);

	/* compute the smallest power of 2 that is greater than the degree */
	size = (int) _dg_nextpow ((uint32_t) src->deg);
	ASSERT (size < INT_MAX);
	ASSERT (size >= 1);
	ASSERT (size >= src->deg);

	/* copy the degree and return if it is 0 */
	dst->deg = src->deg;
	if (src->deg == 0) {
		dst->adj = 0;
		return;
	}

	/* allocate memory for the adjacency array and make a raw copy of the
	 * original */
	dst->adj = gl_malloc (sizeof (struct dg *) * size);
	memcpy (dst->adj, src->adj, src->deg * sizeof (struct dg *));
}

int dg_test (const struct dg * n1, const struct dg * n2)
{
	register int i;

	for (i = 0; i < n1->deg; i++) if (n1->adj[i] == n2) return 1;
	return 0;
}

