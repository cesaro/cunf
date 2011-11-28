
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

#include "global.h"
#include "ls/ls.h"
#include "al/al.h"
#include "debug.h"
#include "glue.h"
#include "h.h"

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
	P ("\tnode    [shape=box style=filled fillcolor=grey60];\n");
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
	ASSERT (sizeof (unsigned long int) == 4);
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
 * Contextual Unfolding Format (CUF)
 *
 * 1. number of conditions
 * 2. number of events
 * 3. number of cutoffs (last events in the list)
 * 4. maximum size of a string in the next two sections
 * 5. list of events; each entry is the label of the event, transition name
 * 6. list of conditions; each entry consist on:
 * 	1. label of the condition, place name
 * 	2. 0 if initial condition; nr. of generating event otherwise
 * 	3. list of postset events
 * 	4. 0 (zero)
 * 	5. list of context events
 * 	6. 0 (zero)
 */

void write_cuf (const char * filename)
{
	FILE * f;
	struct place * p;
	struct trans * t;
	struct event * e;
	struct cond * c;
	struct ls * n;
	int i, ecff;

	/* open file */
	f = fopen (filename, "wb");
	if (! f) gl_err ("'%s': %s", filename, strerror (errno));

	/* reverse the list of events and conditions */
	ls_reverse (&u.unf.events);
	ls_reverse (&u.unf.conds);

	/* count the number of cutoffs */
	ecff = 0;
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		if (e->iscutoff) ecff++;
	}
	ASSERT (i - 1 == u.unf.numev - 1);
	u.unf.numecffs = ecff;

	/* 1. number of conditions */
	_write_int (filename, f, u.unf.numcond);
	
	/* 2. number of events (without e0) */
	_write_int (filename, f, u.unf.numev - 1);

	/* 3. number of cutoffs (last events in the list) */
	_write_int (filename, f, ecff);

	/* 4. maximum size of a string in the next two sections */
	i = 0;
	for (n = u.net.places.next; n; n = n->next) {
		p = ls_i (struct place, n, nod);
		if ((int) strlen (p->name) > i) i = strlen (p->name);
	}
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);
		if ((int) strlen (t->name) > i) i = strlen (t->name);
	}
	_write_int (filename, f, i);

	/* 5. list of events, and re-number: first non-cutoffs, then cutoffs */
	i = 1;
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		if (! e->iscutoff) {
			_write_str (filename, f, e->ft->name);
			e->id = i++;
		}
	}
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->iscutoff) {
			_write_str (filename, f, e->ft->name);
			e->id = i++;
		}
	}

	/* 6. list of conditions, flow and context relation */
	for (n = u.unf.conds.next; n; n = n->next) {
		c = ls_i (struct cond, n, nod);
		_write_str (filename, f, c->fp->name);
		if (c->pre->id != 0) {
			_write_int (filename, f, c->pre->id);
		} else {
			_write_int (filename, f, 0);
		}
		for (i = c->post.deg - 1; i >= 0; i--) {
			e = (struct event *) c->post.adj[i];
			_write_int (filename, f, e->id);
		}
		_write_int (filename, f, 0);
		for (i = c->cont.deg - 1; i >= 0; i--) {
			e = (struct event *) c->cont.adj[i];
			_write_int (filename, f, e->id);
		}
		_write_int (filename, f, 0);
	}

	fclose (f);
}

/*
 * Contextual Unfolding Format (CUF) - old version
 *
 * All integers are 32 bit long, in network byte order (big-endian)
 *
 * 1. number of conditions
 * 2. number of events
 * 3. number of cutoffs (last events in the list)
 * 4. list of events; entries:
 * 	1. number of transition associated to the event
 * 5. list of conditions:
 * 	1. number of the place associated to the condition
 * 	2. 0 if initial condition; nr. of generating event otherwise
 * 	3. list of postset events, as in 4
 * 	4. 0 (zero)
 * 	5. list of context events, as in 4
 * 	6. 0 (zero)
 * 6. number of places in the original net
 * 7. number of transitions in the original net
 * 8. maximum size of a string in the next two sections
 * 9. list of place names
 * 	1. character string storing the name
 * 	2. character '\0'
 * 10. character '\0'
 * 11. list of transitions names, as in 9.
 * 12. character '\0'
 */

void write_cuf_old (const char * filename)
{
	FILE * f;
	struct place * p;
	struct trans * t;
	struct event * e;
	struct cond * c;
	struct ls * n;
	int i, ecff;

	/* open file */
	f = fopen (filename, "wb");
	if (! f) gl_err ("'%s': %s", filename, strerror (errno));

	/* reverse the list of events and conditions */
	ls_reverse (&u.unf.events);
	ls_reverse (&u.unf.conds);

	/* re-number the events: first non-cutoffs then "cutoff events" */
	i = 1;
	ecff = 0;
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;
		for (i = e->hist.deg - 1; i >= 0; i--) {
			if (((struct h *) e->hist.adj[i])->corr == 0) break;
		}
		if (i >= 0) {
			e->id = i++;
		} else {
			e->id = -1;
			ecff++;
		}
	}
	u.unf.numecffs = ecff;
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id < 0) e->id = i++;
	}
	ASSERT (i - 1 == u.unf.numev - 1);

	/* 1. number of conditions */
	_write_int (filename, f, u.unf.numcond);
	
	/* 2. number of events (without e0) */
	_write_int (filename, f, u.unf.numev - 1);

	/* 3. number of cutoffs (last events in the list) */
	_write_int (filename, f, ecff);

	/* 4. list of events */
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id > 0) _write_int (filename, f, e->ft->id);
	}
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		if (e->id < 0) {
			_write_int (filename, f, e->ft->id);
			e->id *= -1;
		}
	}

	/* 5. list of conditions: */
	for (n = u.unf.conds.next; n; n = n->next) {
		c = ls_i (struct cond, n, nod);

		_write_int (filename, f, c->fp->id);
		if (c->pre->id != 0) {
			_write_int (filename, f, c->pre->id);
		} else {
			_write_int (filename, f, 0);
		}
		for (i = c->post.deg - 1; i >= 0; i--) {
			e = (struct event *) c->post.adj[i];
			_write_int (filename, f, e->id);
		}
		_write_int (filename, f, 0);
	}

	/* 6. number of places in the original net */
	_write_int (filename, f, u.net.numpl);

	/* 7. number of transitions in the original net */
	_write_int (filename, f, u.net.numtr);

	/* 8. maximum size of a string in the next two sections */
	i = 0;
	for (n = u.net.places.next; n; n = n->next) {
		p = ls_i (struct place, n, nod);
		if ((int) strlen (p->name) > i) i = strlen (p->name);
	}
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);
		if ((int) strlen (t->name) > i) i = strlen (t->name);
	}
	_write_int (filename, f, i);

	/* 9. list of place names */
	for (n = u.net.places.next; n; n = n->next) {
		p = ls_i (struct place, n, nod);
		_write_str (filename, f, p->name);
	}

	/* 10. character '\0' */
	_write_str (filename, f, "");

	/* 11. list of transitions names, as in 9. */
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);
		_write_str (filename, f, t->name);
	}

	/* 12. character '\0' */
	_write_str (filename, f, "");

	fclose (f);
}
