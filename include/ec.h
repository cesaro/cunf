
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

#define EC_ISGEN(r)	((r)->h != 0 && (r)->c->pre == (r)->h->e)
#define EC_ISREAD(r)	((r)->h != 0 && (r)->c->pre != (r)->h->e)
#define EC_ISCOMP(r)	((r)->h == 0)

/* FIXME -- verify that pre(t) doesn't intersect cont(t) for any t in N */

#endif

