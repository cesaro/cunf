
/*
 * Adjacency List -- implementation
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

#include <string.h>
#include <limits.h>
#include <stdint.h>

#include "glue.h"
#include "al/al.h"

uint32_t _al_nextpow (uint32_t i)
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

static void _al_alloc (struct al * n)
{
	/* ensure: after calling, it's possible to write to positions from 0 to
	 * n->deg (both included) in array n->adj, if the function has been
	 * called with all the values of n->deg, starting at 0, 1 or 2 */

	/* if current size is a power of two */
	if ((n->deg & (n->deg - 1)) == 0) {

		/* 2 slots is the minimum size */
		if (n->adj == 0 || n->deg == 0) {
			n->adj = gl_realloc (0, 2 * sizeof (struct al *));
			return;
		}

		/* duplicate the size */
		n->adj = gl_realloc (n->adj, n->deg * 2 * sizeof (struct al *));
	}
}

void al_init (struct al * n)
{
	ASSERT (n);
	n->deg = 0;
	n->adj = 0;
}

void al_term (struct al * n)
{
	ASSERT (n);
	gl_free (n->adj);
}

void al_add (struct al * n, void * ptr)
{
	ASSERT (n);

	/* realloc (if necessary) the adj array and store the pointer there */
	_al_alloc (n);
	n->adj[n->deg] = ptr;
	n->deg++;
}

void al_rem (struct al * n, const void * ptr)
{
	int i, lst;

	/* search for the pointer in the adjacency list of n and overwrite that
	 * position with the contents of the last item in the list */

	ASSERT (n);

	lst = n->deg - 1;
	for (i = lst; i >= 0; i--) if (n->adj[i] == ptr) break;
	if (i < 0) return;
	n->deg--;
	if (lst != i) n->adj[i] = n->adj[lst];
	_al_alloc (n);
}

void al_cpy (struct al * dst, const struct al * src)
{
	int size;

	ASSERT (dst);
	ASSERT (src);
	ASSERT (dst->deg == 0);

	/* compute the smallest power of 2 that is greater or equal than the
	 * degree */
	size = (int) _al_nextpow ((uint32_t) src->deg);
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
	dst->adj = gl_realloc (0, sizeof (struct al *) * size);
	memcpy (dst->adj, src->adj, src->deg * sizeof (struct al *));
}

int al_test (register const struct al * n, register const void * ptr)
{
	register int i;

	ASSERT (n);

	for (i = 0; i < n->deg; i++) if (n->adj[i] == ptr) return 1;
	return 0;
}

int al_cmp (const struct al * n1, register const struct al * n2)
{
	register int i, j;
	register struct al * p;

	if (n1->deg != n2->deg) return 1;

	for (i = n1->deg - 1; i >= 0; i--) {
		p = n1->adj[i];
		for (j = n2->deg - 1; j >= 0; j--) if (n2->adj[j] == p) break;
		if (j < 0) break;
	}
	if (i < 0) return 0;
	return 1;
}

