
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

#ifndef _EC_H_
#define _EC_H_

#include "global.h"
#include "ls/ls.h"
#include "al/al.h"
#include "h.h"

struct ec {
	struct ls nod;		/* node for the list of ec. associated to c */
	struct cond * c;	/* condition */
	struct al co;		/* list of ec. concurrent to this ec. */
	int m;			/* general purpose mark */

	struct h *h;		/* history for a reading / generating ec. */
	struct ec *r1;		/* pointers to the ec. in case of compound ec */
	struct ec *r2;
};

struct ec * ec_alloc (struct cond * c, struct h * h);
struct ec * ec_alloc2 (struct ec *r1, struct ec *r2);
int ec_included (struct ec *r, struct ec *rp);

#define EC_ISGEN(r)	((r)->h != 0 && (r)->c->pre == (r)->h->e)
#define EC_ISREAD(r)	((r)->h != 0 && (r)->c->pre != (r)->h->e)
#define EC_ISCOMP(r)	((r)->h == 0)

/* FIXME -- verify that pre(t) doesn't intersect cont(t) for any t in N */

#endif

