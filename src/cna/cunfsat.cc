
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <stdexcept>

#include "util/misc.h"
#include "cunf/global.h"
#include "cna/spec.hh"
#include "sat/cnf.hh"

#include "cna/cunfsat.hh"

namespace cna {

Cunfsat::Cunfsat ()
{
	__phi_msat = new sat::Msat (); // we chose minisat here
	phi = __phi_msat;
	event_var_map = new sat::VarMap<struct event *> (*phi);
	place_var_map = new sat::VarMap<struct place *> (*phi);

	spec = 0;
}

Cunfsat::~Cunfsat ()
{
	delete event_var_map;
	delete place_var_map;
	delete __phi_msat;
	delete spec;
}

void Cunfsat::load_spec (const std::string & filename)
{
	FILE * f;

	f = fopen (filename.c_str (), "r");
	if (! f)
	{
		std::string s = fmt ("%s: %s", filename.c_str(), strerror (errno));
		throw std::runtime_error (s);
	}
	if (spec) delete spec;
	spec = spec_parse (f, filename);
}

void Cunfsat::encode ()
{
	ASSERT (spec);
	// FIXME ensure that we have deadlock...
	encode_deadlock ();
}

bool Cunfsat::solve ()
{
	sat::Cnf::result_t ret;

	ret = phi->solve ();
	ASSERT (ret != sat::Cnf::UNK);
	return ret == sat::Cnf::SAT;
}

std::vector<struct event *> & Cunfsat::counterexample ()
{
	sat::CnfModel & model = phi->get_model ();
	struct event * e;
	struct ls * n;
	int m;

	violating_run.clear ();
	m = ++u.mark;
	for (n = u.unf.events.next; n; n = n->next)
	{
		e = ls_i (struct event, n, nod);
		/* we need to explore here exactly the same events that
		 * encode_causality has added, otherwise var (e) will add new
		 * variables */
		if (e->id == 0 || e->iscutoff) continue;
		if (! model[var (e)]) continue;
		e->m = m;
		violating_run.push_back (e);
	}
	return violating_run;
}

void Cunfsat::encode_config ()
{
	encode_causality ();
	encode_sym_conflict ();
	encode_asym_conflict ();
}

void Cunfsat::encode_deadlock ()
{
	encode_config ();
	encode_all_disabled ();
}

void Cunfsat::encode_causality ()
{
	std::vector<struct event *> preds;
	std::vector<sat::Lit> clause;
	struct event * e;
	struct ls * n;

	INFO ("Encoding causality");
	for (n = u.unf.events.next; n; n = n->next)
	{
		e = ls_i (struct event, n, nod);
		if (e->id == 0 || e->iscutoff) continue;
		//if (options.deadlock && is_stubborn (e)) continue;
		get_imm_pred (e, preds);

		if (preds.size () == 0) continue;
		clause.push_back (var (e));
		for (auto it = preds.begin(); it != preds.end(); ++it)
			clause.push_back (var (*it));
		phi->add_clause (clause);
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

	INFO ("Encoding symmetric conflicts");
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
		phi->amo_2tree (conflict);
		conflict.clear ();
	}
}

void Cunfsat::encode_asym_conflict ()
{
	// puff ...
	INFO ("Encoding asymmetric conflicts");
	return;
}

void Cunfsat::encode_all_disabled ()
{
	struct event * e;
	struct cond * c;
	struct ls * n;
	int i, m;
	std::vector<sat::Lit> clause;

	INFO ("Encoding disabledness of all transitions");
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
		phi->add_clause (clause);
		clause.clear ();
	}

	// generate the marking of any condition labelled by a place used above
	INFO ("Encoding marking of every condition");
	for (n = u.unf.conds.next; n; n = n->next)
	{
		c = ls_i (struct cond, n, nod);
		if (c->fp->m == m) encode_mrk_cond (c);
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

void Cunfsat::encode_mrk_cond (struct cond * c)
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
	phi->add_clause (clause);
}

} // namespace cna
