
/*
 * Dynamic Array -- interface and implementation :)
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

#ifndef _DA_DA_H_
#define _DA_DA_H_

#include "glue.h"

struct da {
	int len;
	void * tab;
};

#define da_init(d,t) \
	do { \
		(d)->len = 1; \
		(d)->tab = gl_realloc (0, sizeof (t)); \
	} while (0)

#define da_term(d) \
		gl_free ((d)->tab)

#define da_trunc(d,l,t) \
	do { \
		(d)->len = l; \
		(d)->tab = gl_realloc ((d)->tab, (l) * sizeof (t)); \
	} while (0)

#define da_i(d,i,t) \
		(* (((t *) (d)->tab) + i))

#define da_push(d,i,e,t) \
	do { \
		if ((i) == (d)->len) da_trunc (d, (d)->len << 1, t); \
		da_i (d, i, t) = e; \
		i++; \
	} while (0);

#define da_pop(d,i,t) \
		((i) < (d)->len ? &da_i (d, i++, t) : 0)

#endif
