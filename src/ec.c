
/* 
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

#include "global.h"
#include "ls/ls.h"
#include "al/al.h"
#include "glue.h"
#include "ec.h"
#include "pe.h"
#include "h.h"

static struct ec * _ec_alloc (struct cond * c)
{
	struct ec * r;

	/* mallocate a new structure */
	r = gl_malloc (sizeof (struct ec));

	/* important, append *to the head* to the list c->ecl */
	ls_insert (&c->ecl, &r->nod);

	r->c = c;
	al_init (&r->co);
	r->m = 0;
	return r;
}

struct ec * ec_alloc (struct cond * c, struct h * h)
{
	struct ec *r;

	r = _ec_alloc (c);
	r->h = h;
	r->r1 = r->r2 = 0;
	return r;
}

struct ec * ec_alloc2 (struct ec *r1, struct ec *r2)
{
	struct ec *r;

	ASSERT (r1->c == r2->c);
	r = _ec_alloc (r1->c);
	r->h = 0;
	r->r1 = r1;
	r->r2 = r2;
	return r;
}

int ec_included (struct ec *r, register struct ec *rp)
{
	register int m;

	/* r is a reading ec.; rp is a reading or compound ec.; check whether
	 * one of the readers in rp is included in r's history and return 1 if
	 * yes; in such case, there is another compound ec which can be
	 * combined with r to produce a genuine compound ec. */

	ASSERT (EC_ISREAD (r) && ! EC_ISCOMP (r));
	ASSERT (EC_ISREAD (rp) || EC_ISCOMP (rp));

	m = ++u.mark;
	ASSERT (m > 0);
	h_mark (r->h, m);

	for (; rp->h == 0; rp = rp->r2) {
		ASSERT (rp->r1);
		ASSERT (EC_ISREAD (rp->r1));
		ASSERT (rp->r1->h);
		if (rp->r1->h->m == m) return 1;
	}
	if (rp->h->m == m) return 1;
	return 0;
}
