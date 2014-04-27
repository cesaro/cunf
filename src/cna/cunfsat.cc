
#include "cunf/global.h"
#include "cna/spec.hh"
#include "sat/cnf.hh"

#include "cna/cunfsat.hh"

namespace cna {

void Cunfsat::encode ()
{
}


void Cunfsat::encode_config ()
{
	encode_causality ();
	encode_sym_conflict ();
	encode_asym_conflict ();
}

void Cunfsat::encode_causality ()
{
	std::vector<struct event *> preds;
	std::vector<sat::Lit> clause;
	struct event * e;
	struct ls * n;

	for (n = u.unf.events.next; n; n = n->next)
	{
		e = ls_i (struct event, n, nod);
		if (e->id == 0 || e->iscutoff) continue;
		if (options.deadlock && is_stubborn (e)) continue;
		get_imm_pred (e, preds);

		if (preds.size () == 0) continue;
		clause.push_back (var (e));
		for (auto it = preds.begin(); it != preds.end(); ++it)
			clause.push_back (var (*it));
		phi.add_clause (clause);
		clause.clear ();
		preds.clear ();
	}
}

void Cunfsat::encode_sym_conflict ()
{
	std::vector<sat::Lit> conflict;
	struct event * e;
	struct cond * c;
	struct ls * n;
	int i, j;

	for (n = u.unf.conds.next; n; n = n->next)
	{
		c = ls_i (struct cond, n, nod);

		// first pass, is there more than two non-black events??
		j = 0;
		for (i = c->post.deg - 1; i >= 0; --i)
		{
			e = (struct event *) c->post.adj[i];
			if (! e->iscutoff) {
				j++;
				if (j >= 2) break;
			}
		}
		if (j < 2) continue;

		// second pass, build the list of literals associated to the
		// non-blacks
		for (i = c->post.deg - 1; i >= 0; --i)
		{
			e = (struct event *) c->post.adj[i];
			if (e->iscutoff) continue;
			conflict.push_back (var (e));
		}
		ASSERT (conflict.size () >= 2);
		phi.amo_2tree (conflict);
		conflict.clear ();
	}
}

void Cunfsat::encode_asym_conflict ()
{
	// puff ...
	return;
}

void Cunfsat::encode_deadlock ()
{
	struct event * e;
	struct cond * c;
	struct ls * n;
	int i, m;
	std::vector<sat::Lit> clause;

	// at least one place of every transition's preset is false
	m = ++u.mark;
	for (n = u.unf.events.next; n; n = n->next)
	{
		e = ls_i (struct event, n, nod);
		if (e->id == 0 || e->ft->m == m) continue;
		e->ft->m = m;

		for (i = e->pre.deg - 1; i >= 0; --i)
		{
			c = (struct cond *) e->pre.adj[i];
			clause.push_back (~ var (c->fp));
			c->fp->m = m;
		}
		for (i = e->cont.deg - 1; i >= 0; --i)
		{
			c = (struct cond *) e->cont.adj[i];
			clause.push_back (~ var (c->fp));
			c->fp->m = m;
		}
		phi.add_clause (clause);
		clause.clear ();
	}

	// generate the marking of any condition labelled by a place used above
	for (n = u.unf.conds.next; n; n = n->next)
	{
		c = ls_i (struct cond, n, nod);
		if (c->fp->m == m) encoding_mrk_cond (c);
	}
}

bool Cunfsat::is_stubborn (struct event * e)
{
	return e != e;
}

void
Cunfsat::get_imm_pred (struct event * e, std::vector<struct event *> & l)
{
	int i, m;
	struct cond * c;

	// we skip the bottom event from the list of predecessors

	m = ++u.mark;
	for (i = e->pre.deg - 1; i >= 0; --i)
	{
		c = (struct cond *) e->pre.adj[i];
		if (c->pre->m == m || c->pre->id == 0) continue;
		c->pre->m = m;
		// update here when you implement the stubborn event optimization
		l.push_back (c->pre);
	}

	for (i = e->cont.deg - 1; i >= 0; --i)
	{
		c = (struct cond *) e->cont.adj[i];
		if (c->pre->m == m || c->pre->id == 0) continue;
		c->pre->m = m;
		// and here
		l.push_back (c->pre);
	}
}

void Cunfsat::encoding_mrk_cond (struct cond * c)
{
	/*
	 * c <=> e * ~e1 * ... * ~en
	 *
	 * <=
	 *   (c, ~e, e1, ..., en)
	 *
	 * =>
	 *   (~c, e)
	 *   (~c, ~e1)
	 *   ...
	 *   (~c, ~en)
	 *
	 * but for deadlock checking one only needs the <= direction
	 */

	std::vector<sat::Lit> clause;
	struct event * e;
	int i;

	if (c->pre->iscutoff) return;

	clause.push_back (var (c->fp));
	if (c->pre->id != 0) clause.push_back (~ var (c->pre));
	for (i = c->post.deg - 1; i >= 0; --i)
	{
		e = (struct event *) c->post.adj[i];
		if (! e->iscutoff) clause.push_back (var (e));
	}
	phi.add_clause (clause);
}

} // namespace cna
