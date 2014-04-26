
/* 
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

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "cunf/global.h"
#include "cunf/h.h"
#include "util/ls.h"
#include "util/al.h"
#include "util/debug.h"
#include "util/glue.h"

void write_dot (const char * filename)
{
	struct event *e;
	struct cond *c;
	struct ls *n;
	int i, m;
	FILE *f;

#define P(args...) fprintf (f, ##args)

	f = fopen (filename, "w");
	if (f == 0) {
		gl_err ("'%s': %s", filename, strerror (errno));
		return;
	}

	m = ++u.mark;
	ASSERT (m > 0);
	for (i = u.unf.e0->post.deg - 1; i >= 0; i--) {
		c = (struct cond *) u.unf.e0->post.adj[i];
		c->m = m;
	}

	P ("digraph x {\nnode [shape=box];\n");
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		P ("e%d [label=\"%s:e%d\"];\n", e->id, e->ft->name, e->id);
		u.unf.numepost += e->post.deg;
		u.unf.numecont += e->cont.deg;
		u.unf.numepre += e->pre.deg;
	}

	P ("node [shape=circle];\n");
	for (n = u.unf.conds.next; n; n = n->next) {
		c = ls_i (struct cond, n, nod);
		P (c->m == m ? "c%d [label=\"%s:c%d\"]; /* initial */\n" :
				"c%d [label=\"%s:c%d\"];\n",
				c->id, c->fp->name, c->id);
	}

	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		for (i = e->post.deg - 1; i >= 0; i--) {
			c = (struct cond *) e->post.adj[i];
			P ("e%d -> c%d;\n", e->id, c->id);
		}
	}

	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		for (i = e->pre.deg - 1; i >= 0; i--) {
			c = (struct cond *) e->pre.adj[i];
			P ("c%d -> e%d;\n", c->id, e->id);
		}
		for (i = e->cont.deg - 1; i >= 0; i--) {
			c = (struct cond *) e->cont.adj[i];
			P ("c%d -> e%d [arrowhead=none];\n", c->id, e->id);
		}
	}

	P ("}\n");
	i = fclose (f);
	if (i == EOF) gl_err ("'%s': %s", filename, strerror (errno));
}

void write_dot_fancy (const char * filename)
{
	int i, m, enr, hnr;
	struct dls l, *ln;
	struct h *h, *hp;
	struct event *e;
	struct cond *c;
	struct ls *n;
	FILE *f;

	f = fopen (filename, "w");
	if (f == 0) {
		gl_err ("'%s': %s", filename, strerror (errno));
		return;
	}

	m = ++u.mark;
	ASSERT (m > 0);
	for (i = u.unf.e0->post.deg - 1; i >= 0; i--) {
		c = (struct cond *) u.unf.e0->post.adj[i];
		c->m = m;
	}

	P ("digraph unfolding {\n\t/* events */\n");
	P ("\tnode    [shape=box style=filled fillcolor=gray60];\n");
	enr = 0;
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		enr++;
		P ("\te%-6d [label=\"%s:e%d\"%s];\n",
				e->id,
				e->ft->name,
				e->id,
				e->iscutoff ? " shape=Msquare" : "");
		u.unf.numepost += e->post.deg;
		u.unf.numecont += e->cont.deg;
		u.unf.numepre += e->pre.deg;
	}

	P ("\n\t/* conditions */\n");
	P ("\tnode    [style=filled fillcolor=gray90 shape=circle];\n");
	for (n = u.unf.conds.next; n; n = n->next) {
		c = ls_i (struct cond, n, nod);

		P ("\tc%-6d [label=\"%s:c%d\"];%s\n",
				c->id, c->fp->name, c->id,
				c->m == m ? " /* initial */" : "");
	}

	P ("\n\t/* history annotations for events */\n");
	P ("\tedge    [color=white];\n");
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		P ("\te%-6d -> e%d [label=\"", e->id, e->id);
		for (i = e->hist.deg - 1; i >= 0; i--) {
			h = (struct h *) e->hist.adj[i];
			P ("h%d%s", h->id, i == 0 ? "" : ",");
		}
		P ("\"];\n");
	}

	P ("\n\t/* postset of events */\n");
	P ("\tedge    [color=black];\n");
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;

		for (i = e->post.deg - 1; i >= 0; i--) {
			c = (struct cond *) e->post.adj[i];
			P ("\te%-6d -> c%d;\n", e->id, c->id);
		}
	}

	P ("\n\t/* preset and context of events */\n");
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;

		for (i = e->pre.deg - 1; i >= 0; i--) {
			c = (struct cond *) e->pre.adj[i];
			P ("\tc%-6d -> e%d;\n", c->id, e->id);
		}

		for (i = e->cont.deg - 1; i >= 0; i--) {
			c = (struct cond *) e->cont.adj[i];
			P ("\tc%-6d -> e%d [arrowhead=none color=red];\n",
					c->id, e->id);
		}
	}

	P ("\n\t/* histories */\n");
	P ("\tgraph [fontname=\"Courier\" label=< <br/>\n");
	P ("\tHistory Depth Size Events<br align=\"left\"/>\n");
	hnr = 0;
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		for (i = e->hist.deg - 1; i >= 0; i--) {
			h = (struct h *) e->hist.adj[i];
			hnr++;
			P ("\t%sh%-5d %5d %4d ",
					h->corr ? "*" : " ", h->id,
					h->depth, h->size);
			h_list (&l, h);
			for (ln = l.next; ln; ln = ln->next) {
				hp = dls_i (struct h, ln, auxnod);
				P ("e%d:%s%s ",
						hp->e->id,
						hp->e->ft->name,
						ln->next ? "," : "");
			}
			P ("<br align=\"left\"/>\n");
		}
	}

	ASSERT (enr == u.unf.numev - 1);
	P ("\t <br align=\"left\"/>\n");
	P ("\t%d transitions<br align=\"left\"/>\n"
			"\t%d places<br align=\"left\"/>\n"
			"\t%d events<br align=\"left\"/>\n"
			"\t%d conditions<br align=\"left\"/>\n"
			"\t%d histories<br align=\"left\"/>\n",
			u.net.numtr,
			u.net.numpl,
			u.unf.numev - 1,
			u.unf.numcond,
			u.unf.numh - 1);
#ifdef CONFIG_MCMILLAN
	P ("\tUsing McMillan order<br align=\"left\"/>\n");
#else
	P ("\tUsing Esparza/Romer/Vogler order<br align=\"left\"/>\n");
#endif
	P ("\t>];\n");
	P ("}\n");

	i = fclose (f);
	if (i == EOF) gl_err ("'%s': %s", filename, strerror (errno));
}

static void _write_int (const char * path, FILE * f, unsigned long int i)
{
	int ret;

	i = htonl (i);
	ASSERT (sizeof (unsigned long int) == 4 || 0 == (i >> 32));
	ret = fwrite (&i, 4, 1, f);
	if (ret != 1) gl_err ("'%s': %s", path, strerror (errno));
}

static void _write_str (const char * path, FILE * f, const char * str)
{
	int i;
	
	i = strlen (str) + 1;
	if (fwrite (str, i, 1, f) != 1) gl_err ("'%s': %s", path, strerror (errno));
}

/*
 *  Contextual Unfolding Format (CUF 03)
 *  All integers are 32-bit integers in network byte order
 *  All indexes into any section start at 0
 * 
 *  0. format version number: 0x 43 55 46 03
 *  1. number of places in the original net
 *  2. number of transitions in the original net
 *  3. number of conditions
 *  4. number of events
 *  5. number of white events (events without cutoff histories, they come first
 *     in the list)
 *  6. number of gray events (events with at least one cutoff and one
 *     non-cutoff history, they come after the white events)
 *  7. maximum size of a string in sections 10 and 11
 *  8. list of events; each entry is the index of the associated transition in
 *     the transition table, in section 10; white events are the first, then
 *     gray, the black
 *  9. list of conditions; each entry consist on:
 *  	1. index of the associated place, in section 11
 *  	2. 0xffffffff if initial condition; index of generating event otherwise
 *  	3. postset size
 *  	4. context size
 *  	5. list of indexes of the events in the postset
 *  	6. list of indexes of events in the context
 * 10. list of transitions; each entry consist on the name of the transition,
 *     terminated by a null character
 * 11. list of places; each entry consist on the name of the place, terminated
 *     by a null character
 */

#define PLACE_IDX(p)	(u.net.numpl - 1 - (p)->id)
#define TRANS_IDX(t)	(u.net.numtr - (t)->id)
#define EVENT_IDX(e)	(u.net.numtr - (t)->id)

void write_cuf (const char * filename)
{
	FILE * f;
	struct place * p;
	struct trans * t;
	struct event * e;
	struct cond * c;
	struct ls * n;
	int i, j, nwhite, nblack, mwhite;

	/* generate one color */
	mwhite = ++u.mark;

	/* open file */
	f = fopen (filename, "wb");
	if (! f) gl_err ("'%s': %s", filename, strerror (errno));

	/* reverse the list of events and conditions */
	ls_reverse (&u.unf.events);
	ls_reverse (&u.unf.conds);

	/* count the number of white and black events and color the whites */
	nwhite = 0;
	nblack = 0;
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		if (e->iscutoff) {
			nblack++;
		} else {
			j = 0;
			for (i = e->hist.deg - 1; i >= 0; i--) {
				if (((struct h *) e->hist.adj[i])->corr) j++;
			}
			if (j == 0) {
				nwhite++;
				e->m = mwhite;
			}
		}
		u.unf.numepost += e->post.deg;
		u.unf.numecont += e->cont.deg;
		u.unf.numepre += e->pre.deg;
	}
	u.unf.numeblack = nblack;
	u.unf.numegray = u.unf.numev - 1 - nwhite - nblack;
	u.unf.numewhite = nwhite;

	/* 0. format version number */
	_write_int (filename, f, 0x43554603);

	/* 1. number of places in the original net */
	_write_int (filename, f, u.net.numpl);

	/* 2. number of transitions in the original net */
	_write_int (filename, f, u.net.numtr);

	/* 3. number of conditions */
	_write_int (filename, f, u.unf.numcond);
	
	/* 4. number of events (without e0) */
	_write_int (filename, f, u.unf.numev - 1);

	/* 5. number of white events (events without cutoff histories) */
	_write_int (filename, f, nwhite);

	/* 6. number of gray events (events with at least one cutoff and one *
	 * non-cutoff history) */
	_write_int (filename, f, u.unf.numev - 1 - nwhite - nblack);

	/* 7. maximum size of a string in sections 10 and 11 */
	i = 0;
	for (n = u.net.places.next; n; n = n->next) {
		p = ls_i (struct place, n, nod);
		if ((int) strlen (p->name) > i) i = strlen (p->name);
	}
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);
		if ((int) strlen (t->name) > i) i = strlen (t->name);
	}
	_write_int (filename, f, i + 1);

	/* 8. list of events, and re-number: first white, then gray, then
	 * black */
	i = 0;
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		if (e->m == mwhite) {
			_write_int (filename, f, TRANS_IDX (e->ft));
			e->id = i++;
		}
	}
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id != 0 && e->m != mwhite && ! e->iscutoff) {
			_write_int (filename, f, TRANS_IDX (e->ft));
			e->id = i++;
		}
	}
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->iscutoff) {
			_write_int (filename, f, TRANS_IDX (e->ft));
			e->id = i++;
		}
	}

	/* 9. list of conditions, flow and context relation */
	for (n = u.unf.conds.next; n; n = n->next) {
		c = ls_i (struct cond, n, nod);
		_write_int (filename, f, PLACE_IDX (c->fp));
		if (c->pre->id == 0 && c->pre->m != mwhite) {
			_write_int (filename, f, 0xffffffff);
		} else {
			_write_int (filename, f, c->pre->id);
		}
		_write_int (filename, f, c->post.deg);
		_write_int (filename, f, c->cont.deg);
		for (i = c->post.deg - 1; i >= 0; i--) {
			e = (struct event *) c->post.adj[i];
			_write_int (filename, f, e->id);
		}
		for (i = c->cont.deg - 1; i >= 0; i--) {
			e = (struct event *) c->cont.adj[i];
			_write_int (filename, f, e->id);
		}
	}

	/* 10. list of transitions names (skiping _t0_), terminated with \0 */
	for (n = u.net.trans.next, n = n->next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);
		_write_str (filename, f, t->name);
	}

	/* 11. list of place names, terminated with \0 */
	for (n = u.net.places.next; n; n = n->next) {
		p = ls_i (struct place, n, nod);
		_write_str (filename, f, p->name);
	}

	fclose (f);
}

