
#ifndef _H_H_
#define _H_H_

#include "global.h"
#include "dls.h"
#include "nodelist.h"
#include "dg.h"

struct parikh {
	struct trans *t;
	int count;
};

struct h {
	int id;
	struct event * e;	
	struct dg nod;
	struct dls auxnod;

	int m;
	int size;
	int depth;

	struct nl *marking;
	struct {
		int size;
		struct parikh *tab;
	} parikh;

#ifdef CONFIG_DEBUG
	int debugm;
	struct dls debugnod;
#endif
};

struct h * h_alloc (struct event * e);
struct h * h_dup (struct h * h);
void h_free (struct h *h);

void h_add (struct h * h, struct h * hp);
int h_isdup (const struct h *h);
void h_marking (struct h *h);

int h_conflict (struct h *h1, struct h *h2);
int h_conflict2 (struct h *h1, struct nl *cond1, struct h *h2,
		struct nl *cond2);

void h_list (struct dls *l, struct h *h);
int h_cmp (struct h *h1, struct h *h2);

#endif

