
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

struct h * h_alloc (struct event * e);
struct h * h_dup (struct h * h);
void h_free (struct h *h);

void h_add (struct h * h, struct h * hp);
int h_isdup (struct h *h);
void h_marking (struct h *h);

void h_list (struct dls *l, struct h *h);
int h_cmp (struct h *h1, struct h *h2);

#endif

