
#include "global.h"
#include "debug.h"
#include "glue.h"
#include "ac.h"
#include "dg.h"
#include "h.h"

#include "pe.h"

struct pcomb_entry {
	struct place * p;
	struct nl * n;
};

struct pcomb {
	struct pcomb_entry * tab;
	int size;
	int notdone;
};

struct comb_entry {
	struct cond *c;
	struct event *e;
	int idx;
	struct h *h;
};

struct comb {
	struct h * h;
	struct cond * c;
	struct event * ep;
	struct trans * t;
	struct comb_entry * tab;
	int size;
	int cispre;
};

struct q {
	int size;
	struct h ** tab;
	int skip;
};

struct pe {
	struct q q;
	struct pcomb pcomb;
	struct comb comb;
};
struct pe pe;

/* invariants for the priority queue q:
 * 1. The queue contains exactly size items, from position 1 to position
 *    size, and size+1 slots in the tab array. Position 0 is empty.
 * 2. size is 0 when the queue is empty
 * 3. After calling _pe_alloc, position tab[size] is available for writing
 * 4. For any index with 1 <= index <= size, if index*2 or index*2+1 is <=
 *    size, then tab[index] <= tab[index*2] and tab[index] <= tab[index*2+1]
 */

static void _pe_alloc (void)
{
	/* if current size is a power of two */
	if ((pe.q.size & (pe.q.size - 1)) == 0) {

		/* 2 slots is the minimum size */
		if (pe.q.size == 0) {
			pe.q.tab = (struct h **) gl_realloc (0,
					2 * sizeof (struct h *));
			return;
		}

		/* duplicate the size */
		pe.q.tab = (struct h **) gl_realloc (pe.q.tab,
				pe.q.size * 2 * sizeof (struct h *));
	}
}

static int _pe_hcmp (struct h * h1, struct h * h2)
{
	if (h1->size < h2->size) return -1;
	if (h1->size == h2->size) return 0;
	return 1;
}

static void _pe_insert (struct h * h)
{
	int idx;

	/* if we reached the maximum depth (-d option) or we already found the
	 * stop transition (-T option), skip the insertion */
	/* FIXME -- comparing to the size is not the same as to depth */
	if (u.depth && h->size > u.depth) return;
	if (pe.q.skip) return;
	
	/* when the -T is used and u.stoptr is found, empty the queue to make
	 * sure that the corresponding event is processed immediately. Also,
	 * prevent any further additions to the PE queue. */
	if (h->e->origin == u.stoptr) {
		pe.q.skip = 1;
		pe.q.size = 1;
		pe.q.tab[1] = h;
		return;
	}

	/* ok, make room for a new item and proceed with the insertion */
	pe.q.size++;
	idx = pe.q.size;
	_pe_alloc ();

	/* insert the new element at the end, then move upwards as needed */
	for (; idx > 1; idx /= 2) {
		if (_pe_hcmp (h, pe.q.tab[idx / 2]) > 0) break;
		pe.q.tab[idx] = pe.q.tab[idx / 2]; /* move parent downwards */
	}
	pe.q.tab[idx] = h;
}

static void _pe_update_cont_existing (struct h * h, struct cond * c)
{
	struct event *ep;
	struct h *nh, *hi, *shi;
	int i, j, k;

	/* for each event ep for which c is in preset(ep) */
	for (i = c->post.deg - 1; i >= 0; i--) {
		ep = dg_i (struct event, c->post.adj[i], post);
		
		/* and for each history hi associated to ep */
		for (j = ep->hist.deg - 1; j >= 0; j--) {
			hi = dg_i (struct h, ep->hist.adj[j], nod);

			/* and for each sub-history pointed by hi */
			for (k = hi->nod.deg - 1; k >= 0; k--) {
				shi = dg_i (struct h, hi->nod.adj[k], nod);

				/* if the maximal element of shi is e (actually
				 * h->e), we skip the history hi, as it has one
				 * sub-history shi != h for e that must not be
				 * included as a sub-history for any new
				 * history for ep */
				if (h->e == shi->e) break;

				/* if h is consistent with all shi, we have a
				 * new history for ep */
				if (h_conflict (h, shi)) break;
			}

			/* if consistent with *all*, build the new history for
			 * ep (the same as hi with a new sub-history h) */
			if (k == -1) {
				nh = h_dup (hi);
				h_add (nh, h);
				h_marking (nh);
				_pe_insert (nh);
			}
		}
	}
}

static void _pe_update_cont (struct h * h)
{
	struct cond * c;
	int i;

	/* for each condition c in the context of h->e */
	for (i = h->e->cont.deg - 1; i >= 0; i--) {
		c = dg_i (struct cond, h->e->cont.adj[i], cont);

		/* udpate pe with all the new histories that arise for existing
		 * events ep such that c is in context(ep) */
		_pe_update_cont_existing (h, c);
	}
}

static void _pe_pcomb_init (struct cond * c, struct trans * t)
{
	struct place *p;
	int i;

	pe.pcomb.size = 0;
	pe.pcomb.notdone = 1;
	for (i = t->pre.deg - 1; i >= 0; i--) {
		p = dg_i (struct place, t->pre.adj[i], pre);
		if (p == c->origin) continue;
		if (p->conds == 0) {
			pe.pcomb.notdone = 0;
			return;
		}
		pe.pcomb.tab[pe.pcomb.size].p = p;
		pe.pcomb.tab[pe.pcomb.size].n = p->conds;
		pe.pcomb.size++;
	}
	for (i = t->cont.deg - 1; i >= 0; i--) {
		p = dg_i (struct place, t->cont.adj[i], cont);
		if (p == c->origin) continue;
		if (p->conds == 0) {
			pe.pcomb.notdone = 0;
			return;
		}
		pe.pcomb.tab[pe.pcomb.size].p = p;
		pe.pcomb.tab[pe.pcomb.size].n = p->conds;
		pe.pcomb.size++;
	}
	ASSERT (pe.pcomb.size <= u.net.maxpre + u.net.maxcont);
}

static void _pe_pcomb_next (void)
{
	int idx;

	idx = pe.pcomb.size - 1;
	while (1) {
		/* we already explored all the combinations */
		if (idx < 0) {
			pe.pcomb.notdone = 0;
			return;
		}
		ASSERT (pe.pcomb.tab[idx].n != 0);

		/* move to the next pointer in the last list */
		pe.pcomb.tab[idx].n = pe.pcomb.tab[idx].n->next;

		/* if it is null, reinitialize the list and process the
		 * previous list (idx--); otherwise we are done */
		if (pe.pcomb.tab[idx].n) return;
		pe.pcomb.tab[idx].n = pe.pcomb.tab[idx].p->conds;
		idx--;
	}
}

static void _pe_comb_init (void)
{
	int i, mark;
	struct cond * c;

	ASSERT (pe.pcomb.notdone);

	/* get a new mark */
	u.mark++;
	mark = u.mark;
	ASSERT (mark > 0);

	/* mark the event generating condition pe.comb.c */
	pe.comb.c->pre->m = mark;

	/* for all conditions in the place comb */
	pe.comb.size = 0;
	for (i = 0; i < pe.pcomb.size; i++) {

		/* skip the condition if the event generating it has already
		 * been included in the comb */
		c = pe.pcomb.tab[i].n->node;
		if (c->pre->m == mark) continue;
		c->pre->m = mark;

		pe.comb.tab[pe.comb.size].c = c;
		pe.comb.tab[pe.comb.size].e = c->pre;
		pe.comb.tab[pe.comb.size].idx = c->pre->hist.deg - 1;
		ASSERT (c->pre->hist.deg >= 1);
		pe.comb.size++;
	}
}

static struct event * _pe_new_event (struct trans * t)
{
	int i, max;
	struct event *e;
	struct cond *c;

	/* there is a new occurrence of transition t, firing using condition
	 * pe.comb.c and all conditions in pe.pcomb.tab */

	ASSERT (t);
	ASSERT (t->pre.deg + t->cont.deg == pe.pcomb.size + 1);

	/* allocate and initialize the event */
	e = gl_malloc (sizeof (struct event));
	e->nod.next = (void *) 1; /* FIXME -- !!!! */
	// ls_add (&u.unf.events, &e->nod); -> we do it late, can be a cutoff!
	dg_init (&e->pre);
	dg_init (&e->post);
	dg_init (&e->cont);
	dg_init (&e->ac);
	dg_init (&e->hist);
	e->origin = t;
	e->m = 0;

	e->id = u.unf.numev++;

	/* if pe.comb.c is in preset of the new event */
	if (pe.comb.cispre) {
		/* sic. obscure feature */
		dg_add (&e->pre, (void *) &pe.comb.c->pre);
		dg_add (&pe.comb.c->post, &e->post);
		max = t->pre.deg - 1;
	} else {
		dg_add (&e->cont, &pe.comb.c->cont);
		dg_add (&pe.comb.c->cont, &e->cont);
		max = t->pre.deg;
	}
	ASSERT (max <= pe.pcomb.size);

	/* set up preset */
	for (i = 0; i < max; i++) {
		c = pe.pcomb.tab[i].n->node;
		dg_add (&e->pre, (void *) &c->pre); /* sic. obscure feature */
		dg_add (&c->post, &e->post);
	}

	/* set up context */
	for (; i < pe.pcomb.size; i++) {
		c = pe.pcomb.tab[i].n->node;
		dg_add (&e->cont, &c->cont);
		dg_add (&c->cont, &e->cont);
	}
	ASSERT (e->pre.deg == t->pre.deg);
	ASSERT (e->cont.deg == t->cont.deg);

	/* update the asymmetric conflict relation with this event */
	ac_add (e);

	PRINT ("+ Event e%d:%s; pre {", e->id, e->origin->name);
	for (i = e->pre.deg - 1; i >= 0; i--) {
		c = dg_i (struct cond, e->pre.adj[i], pre);
		PRINT (" c%d:%s", c->id, c->origin->name);
	}
	PRINT (" }; cont {");
	for (i = e->cont.deg - 1; i >= 0; i--) {
		c = dg_i (struct cond, e->cont.adj[i], cont);
		PRINT (" c%d:%s", c->id, c->origin->name);
	}
	PRINT ("}\n");
	/* therefore, at this the moment the event has no postset */
	return e;
}

static struct h * _pe_update_build (void)
{
	struct event *e;
	struct h *h;
	int i, m;

	/* create a new enriched event (a pair event, history) and insert it
	 * into pe */
	ASSERT (pe.comb.t);
	if (pe.comb.ep) ASSERT (pe.comb.t == pe.comb.ep->origin);

	/* if pe.comb.ep is null, we have to create a new event; otherwise we
	 * have to determine if the conditions in the comb are all marked with
	 * the same mark as pe.comb.c, in which case we have to append a new
	 * history for pe.comb.ep, instead of allocating a new event */
	if (pe.comb.ep == 0) {
		e = _pe_new_event (pe.comb.t);
	} else {
		m = pe.comb.c->m;
		for (i = pe.comb.size - 1; i >= 0; i--) {
			if (pe.comb.tab[i].c->m != m) break;
		}
		if (i < 0) {
			e = pe.comb.ep;
		} else {
			e = _pe_new_event (pe.comb.t);
		}
	}

	/* allocate a new history and link it to all the sub-histories in the
	 * comb */
	h = h_alloc (e);
	h_add (h, pe.comb.h);
	for (i = pe.comb.size - 1; i >= 0; i--) h_add (h, pe.comb.tab[i].h);

	/* compute the marking associated to that history, and the size of the
	 * history, and return */
	h_marking (h);
	return h;
}

static int _pe_comb_explore (int idx)
{
	int i, j, ret, maxj;
	struct h * h;

	/* base case: we have already explored all the histories */
	if (idx < 0) return idx;

	/* base case: the current combination of histories is consistent, as
	 * we have explored all the paris of histories */
	if (idx == pe.comb.size) {
		h = _pe_update_build ();
		_pe_insert (h);
		return idx - 1;
	}

	maxj = -1;
	/* for each history h associated to the event pe.comb.tab[idx].e */
	for (i = pe.comb.tab[idx].e->hist.deg - 1; i >= 0; i--) {
		h = dg_i (struct h, pe.comb.tab[idx].e->hist.adj[i], nod);

		/* if h is consistent with the new history pe.comb.h */
		if (h_conflict (pe.comb.h, h)) continue;

		/* and also with all histories below index idx */
		for (j = 0; j < idx; j++) {
			if (h_conflict (pe.comb.tab[j].h, h)) break;
		}

		/* if it is not the case, go to the next history and update
		 * maxj */
		if (j < idx) {
			if (j > maxj) maxj = j;
			continue;
		}

		/* recursively examine histories at tab[idx + 1] */
		pe.comb.tab[idx].h = h;
		ret = _pe_comb_explore (idx + 1);

		/* the return value indicates the new index at wich we have to
		 * continue the exploration */
		if (ret != idx) return ret;
	}

	return maxj;
}

static void _pe_update_post_new (struct h *h, struct cond *c, struct trans *t)
{
	/* put the history that triggers this pe update in pe.comb.h, as well
	 * as the condition we are taking into account (pe.comb.c) (ep is set
	 * to null to force _pe_update_build to create a new event, which will
	 * be an ocurrence of pe.comb.t) */
	ASSERT (t);
	pe.comb.h = h;
	pe.comb.c = c;
	pe.comb.ep = 0;
	pe.comb.t = t;

	/* fill the place comb (pe.pcomb) */
	_pe_pcomb_init (c, t);

	/* we now enumerate all possible combinations of conditions associated
	 * to places in the preset and context of transition ep->origin */
	while (pe.pcomb.notdone) {
		_pe_comb_init ();
		_pe_comb_explore (0);
		_pe_pcomb_next ();
	}
}

static void _pe_update_post_existing (struct h *h, struct cond *c,
		struct event *ep)
{
	int causalmrk;
	int i;
	struct cond *ci;

	ASSERT (ep);
	ASSERT (ep->origin);

	/* we first mark the preset and context of ep */
	u.mark++;
	causalmrk = u.mark;
	ASSERT (causalmrk > 0);
	for (i = ep->pre.deg - 1; i >= 0; i--) {
		ci = dg_i (struct cond, ep->pre.adj[i], pre);
		ci->m = causalmrk;
	}
	for (i = ep->cont.deg - 1; i >= 0; i--) {
		ci = dg_i (struct cond, ep->cont.adj[i], cont);
		ci->m = causalmrk;
	}
	ASSERT (c->m == causalmrk);

	/* write to the comb some important pointers to the structures involved
	 * in this pe update */
	pe.comb.ep = ep;
	pe.comb.h = h;
	pe.comb.c = c;
	pe.comb.t = ep->origin;

	/* fill the place comb (pe.pcomb) */
	_pe_pcomb_init (c, ep->origin);

	/* we now enumerate all possible combinations of conditions associated
	 * to places in the preset and context of transition ep->origin */
	while (pe.pcomb.notdone) {
		_pe_comb_init ();
		_pe_comb_explore (0);
		_pe_pcomb_next ();
	}
}

static void _pe_update_post (struct h * h)
{
	struct cond *c;
	struct trans *t;
	struct event *ep;
	int i, j;
	int visited;

	/* create a new mark for the visited events ep and associated
	 * transitions ep->origin */
	u.mark++;
	visited = u.mark;
	ASSERT (visited > 0);

	/* for each condition c in the postset of h->e */
	for (i = h->e->post.deg - 1; i >= 0; i--) {
		c = dg_i (struct cond, h->e->post.adj[i], post);

		/* and each event ep in the postset of c */
		pe.comb.cispre = 1;
		for (j = c->post.deg - 1; j >= 0; j--) {
			ep = dg_i (struct event, c->post.adj[j], post);

			/* we skip ep if it's already visited, or mark it as
			 * visited (also the associated transition) */
			/* FIXME -- other option is marking the preset of ep,
			 * and we could avoid the existence of field m in ep */
			if (ep->m == visited) continue;
			ep->m = visited;
			ep->origin->m = visited;

			/* update pe with all the new histories with a maximal
			 * event associated to pe->origin.  This histories
			 * either have pe as the maximal event or a new event
			 * associated to pe->origin */
			_pe_update_post_existing (h, c, ep);
		}

		/* and, also, each event ep in the context of c */
		pe.comb.cispre = 0;
		for (j = c->cont.deg - 1; j >= 0; j--) {
			ep = dg_i (struct event, c->cont.adj[j], cont);
			if (ep->m == visited) continue;
			ep->m = visited;
			ep->origin->m = visited;
			_pe_update_post_existing (h, c, ep);
		}
	}

	/* now explore again all conditions c in the postset of h->e */
	for (i = h->e->post.deg - 1; i >= 0; i--) {
		c = dg_i (struct cond, h->e->post.adj[i], post);

		/* and consider the transitions t in the postset of the place
		 * associated to condition c */
		pe.comb.cispre = 1;
		for (j = c->origin->post.deg - 1; j >= 0; j--) {
			t = dg_i (struct trans, c->origin->post.adj[j], post);

			/* skip those transitions which has been already marked
			 * in the previous loop */
			if (t->m == visited) continue;
			t->m = visited;

			_pe_update_post_new (h, c, t);
		}

		pe.comb.cispre = 1;
		for (j = c->origin->cont.deg - 1; j >= 0; j--) {
			t = dg_i (struct trans, c->origin->cont.adj[j], cont);
			if (t->m == visited) continue;
			t->m = visited;
			_pe_update_post_new (h, c, t);
		}
	}
}

void pe_init (void)
{
	int sum;

	/* initialize the pe priority queue */
	pe.q.size = 0;
	pe.q.tab = 0;
	pe.q.skip = 0;

	/* initialize the "place comb" (pcomb) and the "history comb" (comb) */
	sum = u.net.maxcont + u.net.maxpre + 2;
	pe.pcomb.tab = gl_malloc (sizeof (struct pcomb_entry) * sum);
	pe.comb.tab = gl_malloc (sizeof (struct comb_entry) * sum);
}

void pe_term (void)
{
	gl_free (pe.q.tab);
	gl_free (pe.comb.tab);
	gl_free (pe.pcomb.tab);
}

void pe_update (struct h * h)
{
	/* we assume that the postset of h->e is already built, and we use it
	 * to update pe with new histories */

	ASSERT (h);
	ASSERT (h->e);
	ASSERT (h->e->post.deg == h->e->origin->post.deg);

	/* explore the context of h->e and update pe with new histories (but
	 * not new events) */
	_pe_update_cont (h);

	/* explore the postset of h->e and update pe with new histories (and
	 * also events) */
	_pe_update_post (h);
}

struct h * pe_pop (void)
{
	struct h * ret;
	int idx, nidx;
	struct h * lst;

	/* nothing to do if the queue is empty */
	if (pe.q.size == 0) return 0;

	/* the minimal element is at offset 1 in the tab */
	ret = pe.q.tab[1];

	/* get a pointer to the last element and decrement size of the queue */
	lst = pe.q.tab[pe.q.size];
	pe.q.size--;

	/* we want to fill position 1 in the queue */
	idx = 1;
	while (1) {

		/* compute the index of the child of idx with the minimum
		 * weight */
		nidx = idx * 2;
		if (nidx > pe.q.size) break;
		if (nidx < pe.q.size && _pe_hcmp (pe.q.tab[nidx],
				pe.q.tab[nidx + 1]) > 0) nidx++;

		/* if last smaller than (or = to) the minimum, we can stop */
		if (_pe_hcmp (lst, pe.q.tab[nidx]) <= 0) break;

		/* otherwise, tab[nidx] is below lst and its sibling */
		pe.q.tab[idx] = pe.q.tab[nidx];
		idx = nidx;
	}
	pe.q.tab[idx] = lst;

	return ret;
}

