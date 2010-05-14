
/*
 * Directed graph -- implementation
 */

#include <string.h>
#include <limits.h>
#include <stdint.h>

#include "debug.h"
#include "glue.h"

#include "dg.h"

uint32_t _dg_nextpow (uint32_t i)
{
	uint32_t m;
	int a;

	/* next power to 2^31 is 2^32, or 0 mod 2^32 :) */
	if (i & (1 << 31)) return 0;

	/* shift a bits to the right if m >= i, otherwise do it to the left;
	 * and stop when a is zero */
	m = 1 << 16;
	for (a = 8; a; a /= 2) {
		if (m == i) return i;
		if (m > i) m >>= a; else m <<= a;
	}
	if (m == i) return m;
	if (m < i) return m * 2;
	return m;
}

static void _dg_alloc (struct dg * n)
{
	/* ensure: after calling, it's possible to write to positions from 0 to
	 * n->deg (both included) in array n->adj, if the function has been
	 * called with all the values of n->deg, starting at 0, 1 or 2 */

	/* if current size is a power of two */
	if ((n->deg & (n->deg - 1)) == 0) {

		/* 2 slots is the minimum size */
		if (n->adj == 0 || n->deg == 0) {
			n->adj = gl_realloc (0, 2 * sizeof (struct dg *));
			// DEBUG ("node %p: realloc %d > 2 !", n, n->deg);
			return;
		}

		/* duplicate the size */
		n->adj = gl_realloc (n->adj, n->deg * 2 * sizeof (struct dg *));
		// DEBUG ("node %p: realloc %d > %d", n, n->deg, n->deg * 2);
	}
}

static int _dg_test2 (const struct dg * n1, register const struct dg * n2)
{
	register int i;

#ifdef CONFIG_DEBUG
	if (n1->plain_or_heap == -1) ((struct dg *) n1)->plain_or_heap = 0;
	ASSERT (n1->plain_or_heap == 0);
#endif

	i = 1;
	while (1) {
		while (i <= n1->deg && n1->adj[i] <= n2) {
			if (n1->adj[i] == n2) return i;
			i *= 2;
		}
		if (i > n1->deg) i /= 2;
		while (i >= 1 && (i & 1) == 1) i /= 2;
		if (i == 0) return 0;
		i++;
	}
}

void dg_init (struct dg * n)
{
	ASSERT (n);
	n->deg = 0;
	n->adj = 0;
#ifdef CONFIG_DEBUG
	n->plain_or_heap = -1;
#endif
}

void dg_term (struct dg * n)
{
	ASSERT (n);
	gl_free (n->adj);
}

void dg_add (struct dg * n1, struct dg * n2)
{
	ASSERT (n1);
	ASSERT (n2);

#ifdef CONFIG_DEBUG
	if (n1->plain_or_heap == -1) n1->plain_or_heap = 1;
	ASSERT (n1->plain_or_heap == 1);
#endif

	/* realloc (if necessary) the adj array and store the pointer there */
	_dg_alloc (n1);
	n1->adj[n1->deg] = n2;
	n1->deg++;
}

void dg_add2 (struct dg * n1, struct dg * n2)
{
	register int i;

#ifdef CONFIG_DEBUG
	if (n1->plain_or_heap == -1) n1->plain_or_heap = 0;
	ASSERT (n1->plain_or_heap == 0);
#endif

	/* this function assumes that the adj array is sorted as a priority
	 * queue and performs the insertion only if n2 is not already present
	 * in n1->adj */
	ASSERT (n1);
	ASSERT (n2);

	if (dg_test2 (n1, n2)) return;

	/* make room for a new item and proceed with the insertion */
	n1->deg++;
	_dg_alloc (n1);
	i = n1->deg;

	/* insert the new element at the end, then move upwards as needed */
	for (; i > 1; i /= 2) {
		if (n2 > n1->adj[i / 2]) break;
		n1->adj[i] = n1->adj[i / 2]; /* move parent downwards */
	}
	n1->adj[i] = n2;
}

void dg_rem (struct dg * n1, const struct dg * n2)
{
	int i, lst;

#ifdef CONFIG_DEBUG
	if (n1->plain_or_heap == -1) n1->plain_or_heap = 1;
	ASSERT (n1->plain_or_heap == 1);
#endif

	/* search for the node n2 in the adjacency list of n1 and overwrite
	 * that position with the contents of the last item in the list */

	ASSERT (n1);
	ASSERT (n2);
	ASSERT (n1->deg >= 1);

	lst = n1->deg - 1;
	for (i = lst; i >= 0; i--) if (n1->adj[i] == n2) break;
	if (i < 0) return;
	n1->deg--;
	if (lst != i) n1->adj[i] = n1->adj[lst];
	_dg_alloc (n1);
}

void dg_rem2 (struct dg * n1, const struct dg * n2)
{
	int idx, nidx;
	struct dg *lst;

#ifdef CONFIG_DEBUG
	if (n1->plain_or_heap == -1) n1->plain_or_heap = 0;
	ASSERT (n1->plain_or_heap == 0);
#endif

	/* search for the node n2 in the adjacency heap of n1 and udpate the
	 * heap moving upwards the last element */

	ASSERT (n1);
	ASSERT (n2);
	ASSERT (n1->deg >= 1);

	/* search for the requested edge */
	idx = _dg_test2 (n1, n2);
	if (idx == 0) return;

	/* get the last edge of the heap and decrement the degree */
	lst = n1->adj[n1->deg];
	n1->deg--;
	_dg_alloc (n1);

	/* we want to fill position idx in the queue */
	while (1) {

		/* compute the index of the child of idx with the minimum
		 * weight */
		nidx = idx * 2;
		if (nidx > n1->deg) break;
		if (nidx < n1->deg && n1->adj[nidx] > n1->adj[nidx + 1]) nidx++;

		/* if last smaller than (or = to) the minimum, we can stop */
		if (lst <= n1->adj[nidx]) break;

		/* otherwise, n1->adj[nidx] is below lst and its sibling */
		n1->adj[idx] = n1->adj[nidx];
		idx = nidx;
	}
	n1->adj[idx] = lst;
}

void dg_cpy (struct dg * dst, const struct dg * src)
{
	int size;

#ifdef CONFIG_DEBUG
	if (src->plain_or_heap == -1) ((struct dg *) src)->plain_or_heap = 1;
	if (dst->plain_or_heap == -1) ((struct dg *) dst)->plain_or_heap = 1;
	ASSERT (src->plain_or_heap == 1);
	ASSERT (dst->plain_or_heap == 1);
#endif

	ASSERT (dst);
	ASSERT (src);
	ASSERT (dst->deg == 0);

	/* compute the smallest power of 2 that is greater or equal than the
	 * degree */
	size = (int) _dg_nextpow ((uint32_t) src->deg);
	ASSERT (size < INT_MAX);
	ASSERT (size >= 1);
	ASSERT (size >= src->deg);

	/* copy the degree and return if it is 0 */
	/* DEBUG ("dst=%p src=%p deg=%d adj=%p size=%d", dst, src, src->deg,
			src->adj, size); */
	dst->deg = src->deg;
	if (src->deg == 0) {
		dst->adj = 0;
		return;
	}

	/* allocate memory for the adjacency array and make a raw copy of the
	 * original */
	dst->adj = gl_realloc (0, sizeof (struct dg *) * size);
	memcpy (dst->adj, src->adj, src->deg * sizeof (struct dg *));
}

void dg_cpy2 (struct dg * dst, const struct dg * src)
{
	int size;

#ifdef CONFIG_DEBUG
	if (src->plain_or_heap == -1) ((struct dg *) src)->plain_or_heap = 0;
	if (dst->plain_or_heap == -1) ((struct dg *) dst)->plain_or_heap = 0;
	ASSERT (src->plain_or_heap == 0);
	ASSERT (dst->plain_or_heap == 0);
#endif

	ASSERT (dst);
	ASSERT (src);
	ASSERT (dst->deg == 0);

	/* compute the smallest power of 2 that is greater or equal than the
	 * degree plus one */
	size = (int) _dg_nextpow ((uint32_t) src->deg + 1);
	ASSERT (size < INT_MAX);
	ASSERT (size >= 1);
	ASSERT (size >= src->deg);

	/* copy the degree and return if it is 0 */
	DEBUG ("dst=%p src=%p deg=%d adj=%p size=%d", dst, src, src->deg,
			src->adj, size);
	dst->deg = src->deg;
	if (src->deg == 0) {
		dst->adj = 0;
		return;
	}

	/* allocate memory for the adjacency array and make a raw copy of the
	 * original */
	dst->adj = gl_realloc (0, sizeof (struct dg *) * size);
	memcpy (dst->adj, src->adj, (src->deg + 1) * sizeof (struct dg *));
}

int dg_test (const struct dg * n1, const struct dg * n2)
{
	register int i;

#ifdef CONFIG_DEBUG
	if (n1->plain_or_heap == -1) ((struct dg *) n1)->plain_or_heap = 1;
	ASSERT (n1->plain_or_heap == 1);
#endif

	for (i = 0; i < n1->deg; i++) if (n1->adj[i] == n2) return 1;
	return 0;
}

int dg_test2 (const struct dg * n1, register const struct dg * n2)
{
	return _dg_test2 (n1, n2) != 0;
}

int dg_cmp (const struct dg * n1, register const struct dg * n2)
{
	register int i, j;
	register struct dg * p;

#ifdef CONFIG_DEBUG
	if (n1->plain_or_heap == -1) ((struct dg *) n1)->plain_or_heap = 1;
	ASSERT (n1->plain_or_heap == 1);
#endif

	if (n1->deg != n2->deg) return 1;

	for (i = n1->deg - 1; i >= 0; i--) {
		p = n1->adj[i];
		for (j = n2->deg - 1; j >= 0; j--) if (n2->adj[j] == p) break;
		if (j < 0) break;
	}
	if (i < 0) return 0;
	return 1;
}

int dg_cmp2 (const struct dg * n1, const struct dg * n2)
{
	register int i;

#ifdef CONFIG_DEBUG
	if (n1->plain_or_heap == -1) ((struct dg *) n1)->plain_or_heap = 0;
	ASSERT (n1->plain_or_heap == 0);
#endif

	if (n1->deg != n2->deg) return 1;
	for (i = n1->deg - 1; i >= 0; i--) {
		if (! dg_test2 (n2, n1->adj[i])) return 0;
	}
	return 0;
}

