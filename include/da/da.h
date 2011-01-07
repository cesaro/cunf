
/*
 * Dynamic Array -- interface and implementation :)
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
