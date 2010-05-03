
#ifndef _H_H_
#define _H_H_

#include "global.h"
#include "dls.h"
#include "nodelist.h"
#include "dg.h"

struct h {
	struct event * e;	
	struct dg nod;
	struct dls auxnod;

	struct nl *marking;
	int size;
	int id;
	int m;
	/* FIXME -- update the depth for something meaningful... */
};

struct h * h_alloc (struct event * e);
struct h * h_dup (struct h * h);

void h_add (struct h * h, struct h * hp);
void h_marking (struct h *h);

int h_conflict (struct h * h1, struct h * h2);

#endif

