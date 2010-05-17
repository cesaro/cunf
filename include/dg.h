
/*
 * Directed graph -- interface
 */

#ifndef _DG_DG_H_
#define _DG_DG_H_

#include "config.h"

struct dg {
	int deg;
	struct dg ** adj;
#ifdef CONFIG_DEBUG
	int plain_or_heap;
#endif
};

void dg_init (struct dg * n);
void dg_term (struct dg * n);

void dg_add (struct dg * n1, struct dg * n2);
void dg_rem (struct dg * n1, const struct dg * n2);
void dg_cpy (struct dg * dst, const struct dg * src);
int dg_test (const struct dg * n1, const struct dg * n2);
int dg_cmp (const struct dg * n1, const struct dg * n2);

void dg_add2 (struct dg * n1, struct dg * n2);
void dg_rem2 (struct dg * n1, const struct dg * n2);
void dg_cpy2 (struct dg * dst, const struct dg * src);
int dg_test2 (const struct dg * n1, const struct dg * n2);
int dg_cmp2 (const struct dg * n1, const struct dg * n2);

#define dg_item(type,nod,field) \
		((type *) ((void *) (nod) - (void *) &((type *) 0)->field))
#define dg_i dg_item
#endif

