
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


