
/*
 * Directed graph -- interface
 */

#ifndef _DGR_DGR_H_
#define _DGR_DGR_H_

struct dg {
	int deg;
	struct dg ** adj;
};

void dg_init (struct dg * n);
void dg_add (struct dg * n1, struct dg * n2);
void dg_add2 (struct dg * n1, struct dg * n2);
void dg_rem (struct dg * n1, struct dg * n2);
void dg_cpy (struct dg * dst, struct dg * src);
int dg_test (const struct dg * n1, const struct dg * n2);

#define dg_item(type,nod,field) \
		((type *) ((void *) (nod) - (void *) &((type *) 0)->field))
#define dg_i dg_item
#endif

