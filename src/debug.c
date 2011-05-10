
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "config.h"
#include "global.h"
#include "debug.h"
#include "glue.h"
#include "ec.h"
#include "h.h"

void breakme (void)
{
}

void db_net (void)
{
	struct place * p;
	struct trans * t;
	struct ls * n;
	int i;

	PRINT ("Net, %d places, %d transitions\n",
			u.net.numpl,
			u.net.numtr);

	for (n = u.net.places.next; n; n = n->next) {
		p = ls_i (struct place, n, nod);
		PRINT ("   Place, id %d name '%s' %s\n"
				"      pre: { ",
				p->id,
				p->name,
				p->m ? "marked" : "");
		for (i = 0; i < p->pre.deg; i++) {
			t = (struct trans *) p->pre.adj[i];
			PRINT ("%d:%s ", t->id, t->name);
		}
		PRINT ("}\n      post: { ");
		for (i = 0; i < p->post.deg; i++) {
			t = (struct trans *) p->post.adj[i];
			PRINT ("%d:%s ", t->id, t->name);
		}
		PRINT ("}\n      cont: { ");
		for (i = 0; i < p->cont.deg; i++) {
			t = (struct trans *) p->cont.adj[i];
			PRINT ("%d:%s ", t->id, t->name);
		}
		PRINT ("}\n");
	}

	PRINT ("\n");
	for (n = u.net.trans.next; n; n = n->next) {
		t = ls_i (struct trans, n, nod);
		PRINT ("   Trans, id %d name '%s'\n"
				"      pre: size %d { ",
				t->id,
				t->name,
				t->pre.deg);
		for (i = 0; i < t->pre.deg; i++) {
			p = (struct place *) t->pre.adj[i];
			PRINT ("%d:%s ", p->id, p->name);
		}
		PRINT ("}\n      post: size %d { ", t->post.deg);
		for (i = 0; i < t->post.deg; i++) {
			p = (struct place *) t->post.adj[i];
			PRINT ("%d:%s ", p->id, p->name);
		}
		PRINT ("}\n      cont: size %d { ", t->cont.deg);
		for (i = 0; i < t->cont.deg; i++) {
			p = (struct place *) t->cont.adj[i];
			PRINT ("%d:%s ", p->id, p->name);
		}
		PRINT ("}\n");
	}
}

#ifdef CONFIG_DEBUG
void db_h (struct h *h)
{
	static int m = 1;
	struct dls l, *n;
	struct h *hp, *hpp;
	int i, s;

	/* generate a new mark */
	m++;

	dls_init (&l);
	dls_append (&l, &h->debugnod);
	h->debugm = m;

	s = 1;
	for (n = l.next; n; n = n->next) {
		hp = dls_i (struct h, n, debugnod);
		ASSERT (hp->debugm == m);
		for (i = hp->nod.deg - 1; i >= 0; i--) {
			hpp = (struct h *) hp->nod.adj[i];
			if (hpp->debugm == m) continue;
			hpp->debugm = m;
			dls_append (&l, &hpp->debugnod);
			s++;
		}
	}
	/* ASSERT (s == h->size); we might call this function after h is
	 * completely initialized */

/*
 h12/e34:T123; depth 5; size 7; e123:T12, e3:T12, e0:__t0
*/
	PRINT ("  h%d/e%d:%s; depth %d; size %d; ",
			h->id, h->e->id, h->e->ft->name, h->depth, s);
	for (n = l.next; n; n = n->next) {
		hp = dls_i (struct h, n, debugnod);
		PRINT ("e%d:%s, ", hp->e->id, hp->e->ft->name);
	}
	PRINT ("\n");
}

void db_hgraph (void)
{
	struct event *e;
	struct h *h;
	struct ls *n;
	int i;

	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		for (i = e->hist.deg - 1; i >= 0; i--) {
			h = (struct h *) e->hist.adj[i];
			db_h (h);
		}
	}
	PRINT ("\n");
}
#endif

void db_c (struct cond *c)
{
	struct event *e;
	int i;

	PRINT ("  c%d:%s  pre e%d:%s;  post ",
			c->id,
			c->fp->name,
			c->pre->id,
			c->pre->ft->name);

	for (i = c->post.deg - 1; i >= 0; i--) {
		e = (struct event *) c->post.adj[i];
		PRINT ("e%d:%s ", e->id, e->ft->name);
	}

	PRINT ("\b;  cont ");
	for (i = c->cont.deg - 1; i >= 0; i--) {
		e = (struct event *) c->cont.adj[i];
		PRINT ("e%d:%s ", e->id, e->ft->name);
	}
	PRINT ("\b;\n");
}

void db_e (struct event *e)
{
	struct cond *c;
	int i;

	PRINT ("  e%d:%s  pre ",
			e->id,
			e->ft->name);

	for (i = e->pre.deg - 1; i >= 0; i--) {
		c = (struct cond *) e->pre.adj[i];
		PRINT ("c%d:%s ", c->id, c->fp->name);
	}
	PRINT ("\b;  post ");

	for (i = e->post.deg - 1; i >= 0; i--) {
		c = (struct cond *) e->post.adj[i];
		PRINT ("c%d:%s ", c->id, c->fp->name);
	}

	PRINT ("\b;  cont ");
	for (i = e->cont.deg - 1; i >= 0; i--) {
		c = (struct cond *) e->cont.adj[i];
		PRINT ("c%d:%s ", c->id, c->fp->name);
	}
	PRINT ("\b;\n");
}

static void _db_r (int spc, struct ec *r) {
/*
  {c0:P12, -} type C
    {c0:P12, h12/e1:T123} type [R|G]
    [c0:P12, -] type C
      [c0:P12, h12/e1:T123] type [R|G]
      [c0:P12, h12/e1:T123] type [R|G]
*/	

	int i;

	ASSERT (EC_PTR (r) == r);
	ASSERT (r);
	ASSERT (r->c);
	ASSERT (r->c->fp);

	for (i = 0; i < spc; i++) PRINT (" ");
	PRINT ("  {c%d:%s, ", r->c->id, r->c->fp->name);

	if (EC_ISCOMP (r)) {
		ASSERT (r->h == 0);
		ASSERT (r->r1);
		ASSERT (r->r2);
		ASSERT (! EC_ISCOMP (r->r1));

		PRINT ("-} type C\n");
		_db_r (spc + 2, r->r1);
		_db_r (spc + 2, r->r2);
	} else {
		ASSERT (r->h);
		ASSERT (r->h->e);
		ASSERT (r->h->e->ft);
		ASSERT (EC_ISGEN (r) || EC_ISREAD (r));
		PRINT ("h%d/e%d:%s} type %s\n",
			r->h->id,
			r->h->e->id,
			r->h->e->ft->name,
			EC_ISGEN (r) ? "G" : "R");
	}
}

void db_r (struct ec *r) {
	_db_r (0, r);
}

void db_r2 (const char *str1, struct ec *r, const char *str2) {
	struct ec *rp;
	int bit0, bit1;

	bit0 = EC_BIT0 (r);
	bit1 = EC_BIT1 (r);
	r = EC_PTR (r);
	ASSERT (r);
	ASSERT (r->c);
	ASSERT (r->c->fp);

	PRINT ("%s%s{c%d:%s, ", str1 ? str1 : "", bit0 ? "*" : "", r->c->id,
			r->c->fp->name);

	for (rp = r; rp->h == 0; rp = rp->r2) {
		ASSERT (rp->r1);
		ASSERT (EC_ISREAD (rp->r1));
		PRINT (" h%d/e%d:%s",
			rp->r1->h->id,
			rp->r1->h->e->id,
			rp->r1->h->e->ft->name);
	}
	ASSERT (EC_ISREAD (rp) || EC_ISGEN (rp));
	PRINT (" h%d/e%d:%s}%s%s",
		rp->h->id,
		rp->h->e->id,
		rp->h->e->ft->name,
		bit1 ? "*" : "",
		str2 ? str2 : "\n");
}

void db_h2dot (void)
{
	struct event *e;
	struct h *h, *sh;
	struct ls *n;
	int i, j;
	
	printf ("\ndigraph h {\n");
	for (n = u.unf.events.next; n; n = n->next) {
		e = ls_i (struct event, n, nod);
		printf (" \"%se%d:%s\" [shape=diamond style=filled fillcolor=gray90]\n",
				e->iscutoff ? "*" : "",
				e->id, e->ft->name);
		for (i = e->hist.deg - 1; i >= 0; i--) {
			h = (struct h *) e->hist.adj[i];
			ASSERT (e == h->e);
			printf (" \"%se%d:%s\" -> \"%sh%d/e%d:%s\" [arrowhead=dot]\n",
					e->iscutoff ? "*" : "",
					e->id, e->ft->name,
					h->marking ? "" : "*",
					h->id, e->id, e->ft->name);
			for (j = h->nod.deg - 1; j >= 0; j--) {
				sh = (struct h *) h->nod.adj[j];
				ASSERT (sh->marking != 0);
				printf (" \"%sh%d/e%d:%s\" -> \"h%d/e%d:%s\"\n",
						h->marking ? "" : "*",
						h->id,
						h->e->id,
						h->e->ft->name,
						sh->id,
						sh->e->id,
						sh->e->ft->name);
			}
		}
	}
	printf ("}\n\n");
}

void db_mem (void)
{
	char buff[4096];
	int fd, ret;

	fd = open ("/proc/self/status", O_RDONLY);
	if (fd < 0) {
		gl_warn ("'/proc/self/status': %s", strerror (errno));
		return;
	}

	ret = read (fd, buff, 4096);
	close (fd);
	if (ret >= 4096) {
		PRINT ("Bug in db_mem !!\n");
		exit (1);
	}
	buff[ret] = 0;
	PRINT ("%s", buff);
}
