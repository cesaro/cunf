
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <stdexcept>

#include "util/misc.h"
#include "cunf/global.h"
#include "cna/spec.hh"
#include "sat/cnf.hh"

#include "cna/cunfsat.hh"


/*
 *
 * Plain Encoding
 * ==============
 *
 * Def: E is the set of events
 * Def: Ec is E minus all cutoff (black) events
 * Def: pred(e) = pre ( pre(e) U cont(e) )
 * Def: ev(t) = {e in E : t is the label of e}
 *
 * Variables
 * 	e		event e happens
 * 	t		transition t is enabled
 * 	e*		event e is is enabled at the configuration identified by the e's
 * 	dead	a single variable saying whether the configuration is a dead
 *
 * Cusality
 * 	for all e in Ec and all e' in pred (e), generate
 * 	e => e'
 *
 * Symmetric conflict
 * 	for all conditions c with at least two events in both post(c) and Ec
 * 	AMO (e_1, ..., e_n) [with e_i in post(c) \cap Ec]
 *
 * Deadlock (part C)
 * 	dead <=> ~e_1* ^ ... ^ ~e_n* [with e_i in E]
 *
 * 	Part C=>
 * 		(~dead, ~e_1*), ..., (~dead, ~e_n*)
 * 	Part C<=
 * 		(e_1*, ..., e_n*, dead)
 *
 * Enabled transitions (part A)
 * 	for all transition t happening in the specification, generate
 * 	t <=> e_1* v ... v e_n*  [with e_i in ev(t)]
 * 	- if ev(t) is empty, part A<= will not produce any transition and
 * 	  part A=> will set a clause ensuring "t <=> false"
 *
 * 	Part A=>
 * 		(~t, e_1*, ..., e_n*)
 * 	Part A<=
 * 		(~e_1*, t), ..., (~e_n*, t)
 *
 * Enabled events (part B)
 * 	for all events e such that e* was referenced in part A or C, generate
 * 	e* <=> ~e ^ e_1 ^ ... ^ e_n [with e_i in pred(e)]
 *		- if e is cutoff, omit e in the rhs (as ~e is always true)
 *
 *		Part B=>
 *			(~e*, ~e), (~e*, e_1), ..., (~e*, e_n)
 *		Part B<=
 *			(e, ~e_1, ..., ~e_n, e*)
 *
 * You need,
 * 	- for every transition happening only as a positive atom:
 * 		parts A=>, B=>
 * 	- for every transition happenign only as a negative atom:
 * 		parts A<=, B<=
 * 	- for transitions happening with both signs: both half-parts
 *
 * 	- for deadlock happening only as a positive atom:
 * 		parts C=>, B<=
 * 	- as only a negative atom
 * 		parts C<=, B=>
 * 	- both half-parts if it hapens with both signs
 * 	
 */

using namespace cna;

Cunfsat::Cunfsat (Spec & s)
	: spec (s)
{
	__phi_msat = new sat::Msat (); // we chose minisat here
	phi = __phi_msat;
	event_var_map = new sat::VarMap<struct event *> (*phi);
	event_en_var_map = new sat::VarMap<struct event *> (*phi);
	place_var_map = new sat::VarMap<struct place *> (*phi);
	trans_var_map = new sat::VarMap<struct trans *> (*phi);
}

Cunfsat::~Cunfsat ()
{
	delete event_var_map;
	delete event_en_var_map;
	delete place_var_map;
	delete trans_var_map;
	delete __phi_msat;
}

void Cunfsat::encode ()
{
	encode_plain ();
	INFO ("SAT encoding: %d variables, %d clauses, using Minisat",
			phi->no_vars (), phi->no_clauses ());
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
		 * encode_ctx_causality has added, otherwise var (e) will add new
		 * variables */
		if (e->id == 0 || e->iscutoff) continue;
		if (! model[var (e)]) continue;
		e->m = m;
		violating_run.push_back (e);
	}

	// assert that the answer is indeed a sound answer
	assert_is_deadlock (m);

	return violating_run;
}

void Cunfsat::encode_ctx_config ()
{
	encode_ctx_causality ();
	encode_ctx_sym_conflict ();
	encode_ctx_asym_conflict ();
}

void Cunfsat::encode_ctx_deadlock ()
{
	encode_ctx_config ();
	encode_ctx_all_disabled ();
}

void Cunfsat::encode_ctx_causality ()
{
	std::vector<struct event *> preds;
	std::vector<sat::Lit> clause (2);
	struct event * e;
	struct ls * n;

	INFO ("Encoding causality (ctx version)");
	for (n = u.unf.events.next; n; n = n->next)
	{
		e = ls_i (struct event, n, nod);
		if (e->id == 0 || e->iscutoff) continue;
		//if (options.deadlock && is_stubborn (e)) continue;
		get_imm_pred (e, preds);

		clause[0] = ~ var (e);
		for (auto it = preds.begin(); it != preds.end(); ++it)
		{
			clause[1] = var (*it);
			phi->add_clause (clause);
		}
		preds.clear ();
	}
}

void Cunfsat::encode_ctx_sym_conflict ()
{
	std::vector<sat::Lit> conflict;
	struct event * e;
	struct cond * c;
	struct ls * n;
	int i, j;

	INFO ("Encoding symmetric conflicts (ctx version)");
	for (n = u.unf.conds.next; n; n = n->next)
	{
		c = ls_i (struct cond, n, nod);

		// first pass, is there more than two non-black events??
		j = 0;
		for (i = c->post.deg - 1; i >= 0; --i)
		{
			e = (struct event *) c->post.adj[i];
			if (! e->iscutoff)
			{
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

void Cunfsat::encode_ctx_asym_conflict ()
{
	// puff ...
	INFO ("Encoding asymmetric conflicts");
	return;
}

void Cunfsat::encode_ctx_all_disabled ()
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
		if (c->fp->m == m) encode_ctx_mrk_cond (c);
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

void Cunfsat::encode_ctx_mrk_cond (struct cond * c)
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

void Cunfsat::assert_is_config (int m)
{
	struct ls * n;
	struct event * e;
	struct cond * c;
	int i, j;
	
	// check it is causally closed
	for (n = u.unf.events.next; n; n = n->next)
	{
		e = ls_i (struct event, n, nod);
		if (e->m != m) continue;
		for (i = e->pre.deg - 1; i >= 0; --i)
		{
			c = (struct cond *) e->pre.adj[i];
			ASSERT (c->pre->id == 0 || c->pre->m == m);
		}
		for (i = e->cont.deg - 1; i >= 0; --i)
		{
			c = (struct cond *) e->cont.adj[i];
			ASSERT (c->pre->id == 0 || c->pre->m == m);
		}
	}

	// has no symmetric conflict
	for (n = u.unf.conds.next; n; n = n->next)
	{
		c = ls_i (struct cond, n, nod);
		j = 0;
		for (i = c->post.deg - 1; i >= 0; --i)
		{
			e = (struct event *) c->post.adj[i];
			if (e->m == m) j++;
		}
		ASSERT (j <= 1);
	}

	// FIXME assert it has no loop of asymmetric conflict
}

void Cunfsat::assert_is_deadlock (int m)
{
	assert_is_config (m);
	assert_disables_all (m);
}

void
Cunfsat::assert_get_marking (int m, std::unordered_set<struct place *> & mrk)
{
	struct ls * n;
	struct event * e;
	struct cond * c;
	int i, newm;

	// mark all conditions consumed by the configuration
	newm = ++u.mark;
	for (n = u.unf.events.next; n; n = n->next)
	{
		e = ls_i (struct event, n, nod);
		if (e->m != m) continue;
		for (i = e->pre.deg - 1; i >= 0; --i)
		{
			c = (struct cond *) e->pre.adj[i];
			c->m = newm;
		}
	}

	// store all places labelling a condition produced and not consumed
	for (n = u.unf.events.next; n; n = n->next)
	{
		e = ls_i (struct event, n, nod);
		if (e->m != m) continue;
		for (i = e->post.deg - 1; i >= 0; --i)
		{
			c = (struct cond *) e->post.adj[i];
			if (c->m == newm) continue;
			ASSERT (mrk.count (c->fp) == 0); // otherwise the net is not 1-safe
			mrk.insert (c->fp);
		}
	}
}

void Cunfsat::assert_disables_all (int m)
{
	std::unordered_set<struct place *> mrk;
	struct trans * t;
	struct ls * n;

	// compute the marking of the configuration and assert it disables all
	// transitions
	assert_get_marking (m, mrk);
	for (n = u.net.trans.next; n; n = n->next)
	{
		t = ls_i (struct trans, n, nod);
		if (t->id == 0) continue;
		assert_disables_t (mrk, t);
	}
}

void Cunfsat::assert_disables_t
		(std::unordered_set<struct place *> & mrk, struct trans * t)
{
		int i;
		struct place * p;

		for (i = t->pre.deg - 1; i >= 0; --i)
		{
			p = (struct place *) t->pre.adj[i];
			if (mrk.count (p) == 0) return;
		}
		for (i = t->cont.deg - 1; i >= 0; --i)
		{
			p = (struct place *) t->cont.adj[i];
			if (mrk.count (p) == 0) return;
		}
		ASSERT (0);
}

void Cunfsat::mark_atoms (Spec & s)
{
	switch (s.type)
	{
	case Spec::TRANS :
		TRACE ("Atom '%s' happens positive", s.trans->name);
		if (s.trans->m == mrk_pos) return;
		if (s.trans->m == mrk_both) return;
		if (s.trans->m == mrk_neg) { s.trans->m = mrk_both; return; }
		s.trans->m = mrk_pos;
		atoms.push_back (s.trans);
		return;

	case Spec::DEADLOCK :
		TRACE ("Atom 'deadlock' happens positive");
		deadlock_pos = true;
		return;

	case Spec::PLACE :
		// FIXME
		throw std::runtime_error ("Places not supported in the specification");
	case Spec::AND :
	case Spec::OR :
		mark_atoms (* s.left);
		mark_atoms (* s.right);
		return;
	default :
		break;
	}

	// the only remaining case is a negation
	ASSERT (s.type == Spec::NOT);
	ASSERT (s.left && !s.right);
	switch (s.left->type)
	{
	case Spec::TRANS :
		TRACE ("Atom '%s' happens negative", s.left->trans->name);
		if (s.left->trans->m == mrk_neg) return;
		if (s.left->trans->m == mrk_both) return;
		if (s.left->trans->m == mrk_pos) { s.left->trans->m = mrk_both; return; }
		s.left->trans->m = mrk_neg;
		atoms.push_back (s.left->trans);
		return;

	case Spec::DEADLOCK :
		TRACE ("Atom 'deadlock' happens negative");
		deadlock_neg = true;
		return;

	case Spec::AND :
	case Spec::OR :
		mark_atoms (* s.left);
		mark_atoms (* s.right);
		return;
	case Spec::PLACE :
		// FIXME
		throw std::runtime_error ("Places not supported in the specification");
	default :
		ASSERT (0);
	}
}

void Cunfsat::encode_plain ()
{
	mrk_pos = ++u.mark;
	mrk_neg = ++u.mark;
	mrk_both = ++u.mark;
	deadlock_pos = false;
	deadlock_neg = false;

	// transform the specification into negation normal form
	TRACE ("Translating the specification into negation normal form");
	spec.to_nnf ();
	std::string s; spec.str (s);
	TRACE ("Specification in NNF: '%s'", s.c_str ());

	// mark the atoms and store pointers to them in "atoms"
	TRACE ("Exploring the atoms of the specification");
	mark_atoms (spec);

	// encode configurations
	encode_plain_causality ();
	encode_plain_sym_conflict ();

	// encode the constraints associated to the transition atoms
	encode_plain_trans_enabled ();
	encode_plain_event_enabled ();
}

void Cunfsat::encode_plain_trans_enabled ()
{
	struct ls * n;
	struct trans * t;
	struct event * e;
	std::vector<sat::Lit> clause, clause2 (2);

	TRACE ("Encoding atomic constraints: t <=> \\/_ev(t)");
	SHOW (mrk_pos, "d");
	SHOW (mrk_neg, "d");
	SHOW (mrk_both, "d");
	// for all transitions happening in the specification
	for (auto it = atoms.begin (); it != atoms.end (); ++it)
	{
		t = *it;
		clause2[1] = var(t);
		clause.push_back (~ clause2[1]);
		SHOW (t->name, "s");
		SHOW (t->m, "d");

		// for all events labelled by t (if there is at least one...)
		for (n = t->events.next; n; n = n->next)
		{
			e = ls_i (struct event, n, tnod);
			ASSERT (e->ft == t);
			e->m = t->m;

			// if the transition happens in a positive atom, we need Part A=>
			if (t->m == mrk_pos || t->m == mrk_both) clause.push_back (var_en (e));
			if (t->m == mrk_neg || t->m == mrk_both)
			{
				// if it happens in a negative atom, we need part A<=
				clause2[0] = ~ var_en (e);
				phi->add_clause (clause2);
			}
		}

		// add the only clause generated for part A=>
		if (t->m == mrk_pos || t->m == mrk_both) phi->add_clause (clause);
	}
}

void Cunfsat::encode_plain_event_enabled ()
{
	TRACE ("Encoding 'event enabled' constraints: e* <=> ~e ^ /\\_pred(e)");
	for (auto it = event_en_var_map->begin (); it != event_en_var_map->end (); ++it)
	{
		SHOW (it->first->ft->name, "s");
		SHOW (it->second.to_dimacs (), "d");
	}
	// FIXME Continue here!!
}

void Cunfsat::encode_plain_deadlock ()
{
}

void Cunfsat::encode_plain_causality ()
{
	encode_ctx_causality ();
}

void Cunfsat::encode_plain_sym_conflict ()
{
	encode_ctx_sym_conflict ();
}

void Cunfsat::encode_plain_spec ()
{
}

