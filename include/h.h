
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

#ifndef _H_H_
#define _H_H_

#include "nodelist.h"
#include "dls/dls.h"
#include "global.h"
#include "al/al.h"
// #include "glue.h"
#include "config.h"

struct parikh {
	struct trans *t;
	int count;
};

struct h {
	int id;			/* internal history identifier */
	struct event * e;	/* h is associated to this event */
	struct al nod;		/* adjacent histories */
	struct dls auxnod; 	/* FIXME -- do we really need this? */
	struct al ecl;		/* enriched conditions conforming h */

	int m;			/* general purpose mark */
	int size;		/* numer of events in the history */
	int depth;		/* 1 + max(h->nod.adj[i]->depth) */

	struct al rd;		/* events in h reading the cut of h */
	struct al sd;		/* events in h reading from pre(h->e) */
	struct nl *marking;	/* marking associated to the history */
	int hash;		/* hash value of the marking (see marking_hash) */
	struct h *corr;		/* if h is cutoff, corresponding history */
	struct {
		int size;
		struct parikh *tab;
	} parikh;		/* we need this to compute the <_F order */

#ifdef CONFIG_DEBUG
	int debugm;
	struct dls debugnod;
#endif
};

void h_init (void);
void h_term (void);
struct h * h_alloc (struct event * e);
void h_free (struct h *h);

void h_add (struct h * h, struct h * hp);
void h_marking (struct h *h);
void h_list (struct dls *l, struct h *h);
int h_cmp (struct h *h1, struct h *h2);

#endif

