
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <stdexcept>

#include "cunf/global.h"
#include "util/misc.h"
#include "util/stopwatch.hh"
#include "cna/spec.hh"
#include "sat/cnf.hh"

#include "cna/cunfsat.hh"


/*
 *
 * Plain Encoding
 * ==============
 *
 * Def: E is the set of events
 * Def: C is the set of conditions
 * Def: Ec is E minus all cutoff (black) events
 * Def: Cc is the set of conditions having at least two non-cutoffs in post(c)
 * Def: pred(e) = pre ( pre(e) U cont(e) )
 * Def: ev(t) = {e in E : t is the label of e}
 * Def: cf(e) = {c in Cc : c is in pre(e)}
 *
 * Variables
 * 	e		event e happens
 * 	c		exactly one event in post(c) has happened
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
 * 	c <=> AMO (e_1, ..., e_n) [with e_i in post(c) \cap Ec]
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
 * 	e* <=> ~e ^ e_1 ^ ... ^ e_n ^ ~c_1 ^ ... ^ ~c_m
 * 			[with e_i in pred(e) and c_i in cf(e)]
 *		- if e is cutoff, omit e in the rhs (as ~e is always true)
 *
 *		Part B=>
 *			(~e*, ~e), (~e*, e_1), ..., (~e*, e_n), (~e*, ~c_1), ...
 *		Part B<=
 *			(e, ~e_1, ..., ~e_n, c_1, ..., c_m, e*)
 *
 * You need,
 * 	- for every transition happening only as a positive atom:
 * 		parts A=>, B=>
 * 	- for every transition happenign only as a negative atom:
 * 		parts A<=, B<=
 * 	- for transitions happening with both signs: both half-parts
 *
 * 	- for deadlock happening as a positive atom, we need
 * 		parts C=>, B<=
 * 	- as a negative atom, we need
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
	cond_var_map = new sat::VarMap<struct cond *> (*phi);
	event_en_var_map = new sat::VarMap<struct event *> (*phi);
	place_var_map = new sat::VarMap<struct place *> (*phi);
	trans_var_map = new sat::VarMap<struct trans *> (*phi);
}

Cunfsat::~Cunfsat ()
{
	delete event_var_map;
	delete cond_var_map;
	delete event_en_var_map;
	delete place_var_map;
	delete trans_var_map;
	delete __phi_msat;
}

void Cunfsat::encode ()
{
	if (! u.net.isplain)
		throw std::runtime_error ("Input has read arcs, the current SAT " \
				"encoding does not support them");
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
	int m;

	violating_run.clear ();
	m = ++u.mark;
	for (auto it = event_var_map->begin (); it != event_var_map->end (); ++it)
	{
		// it->first is the event; it->second is the variable
		if (model[it->second]) {
			it->first->m = m;
			violating_run.push_back (it->first);
		}
	}

	// assert that the answer is indeed a sound answer
	assert_is_config (m);
	if (spec.type == Spec::DEADLOCK) assert_disables_all (m);

	return violating_run;
}

void Cunfsat::encode_ctx_deadlock ()
{
	encode_causality ();
	encode_sym_conflict ();
	encode_ctx_asym_conflict ();
	encode_ctx_all_disabled ();
}

void Cunfsat::encode_causality ()
{
	std::vector<struct event *> preds;
	std::vector<sat::Lit> clause (2);
	struct event * e;
	struct ls * n;

	INFO (" + Encoding causality");
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

void Cunfsat::encode_sym_conflict ()
{
	std::vector<sat::Lit> conflict;
	struct event * e;
	struct cond * c;
	struct ls * n;
	int i, j;

	INFO (" + Encoding symmetric conflicts");
	for (n = u.unf.conds.next; n; n = n->next)
	{
		c = ls_i (struct cond, n, nod);

		// first pass, is there more than two non-black events??
		j = 0;
		for (i = c->post.deg - 1; i >= 0; --i)
		{
			e = (struct event *) c->post.adj[i];
			if (! e->iscutoff && ++j >= 2) break;
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
		phi->amo_2tree_two_ways (conflict, var (c));
		conflict.clear ();
	}
}

void Cunfsat::encode_ctx_asym_conflict ()
{
	// puff ...
	INFO ("Encoding asymmetric conflicts");
	ASSERT (false);
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
	ASSERT (false);
	return e + 2 != e + 2;
}

void
Cunfsat::get_imm_pred (struct event * e, std::vector<struct event *> & l)
{
	int i, m;
	struct cond * c;

	// we skip the bottom event from the list of predecessors
	// XXX - important! Observe that we do not clear() the list l of events!!!
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

void
Cunfsat::get_imm_sym_cfl (struct event * e, std::vector<struct event *> & l)
{
	struct cond * c;
	struct event * ee;
	int m, i, j;

	// XXX - important! Observe that we do not clear() the list l of events!!!
	m = ++u.mark;
	e->m = m;
	for (i = e->pre.deg - 1; i >= 0; --i)
	{
		c = (struct cond *) e->pre.adj[i];

		for (j = c->post.deg - 1; j >= 0; --j)
		{
			ee = (struct event *) c->post.adj[j];
			if (ee->iscutoff || ee->m == m) continue;
			ee->m = m;
			l.push_back (ee);
		}
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
#ifdef VERB_LEVEL_TRACE
	std::string s;
	spec.to_str (s);
	TRACE ("Specification in NNF: '%s'", s.c_str ());
#endif

	// mark the atoms and store pointers to them in "atoms"
	TRACE (" + Exploring atoms in the specification");
	mark_atoms (spec);

	// encode configurations
	encode_causality ();
	encode_sym_conflict ();

	// encode the constraints associated to the transition atoms
	encode_plain_trans_enabled ();
	encode_plain_deadlock ();
	encode_plain_event_enabled ();

	// finally, encode the specification
	TRACE (" + Encoding the property");
	sat::Lit p = encode_spec (spec);
	phi->add_clause (p);
}

void Cunfsat::encode_plain_trans_enabled ()
{
	struct ls * n;
	struct trans * t;
	struct event * e;
	bool lr;
	bool rl;
	std::vector<sat::Lit> clause, clause2 (2);

	TRACE (" + Encoding atomic constraints: t <=> \\/_ev(t), for %d atoms",
			atoms.size ());
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

		lr = t->m == mrk_pos || t->m == mrk_both;
		rl = t->m == mrk_neg || t->m == mrk_both;

		// for all events labelled by t (if there is at least one...)
		for (n = t->events.next; n; n = n->next)
		{
			e = ls_i (struct event, n, tnod);
			ASSERT (e->ft == t);

			// if the transition happens in a positive atom, we need Part A=>
			if (lr) clause.push_back (var_en (e));
			if (rl)
			{
				// if it happens in a negative atom, we need part A<=
				clause2[0] = ~ var_en (e);
				phi->add_clause (clause2);
			}
		}

		// add the only clause generated for part A=>
		if (lr) phi->add_clause (clause);
		clause.clear ();
	}
}

void Cunfsat::encode_plain_deadlock ()
{
	struct event * e;
	struct ls * n;
	std::vector<sat::Lit> clause, clause2 (2);

	if (! deadlock_pos && ! deadlock_neg)
	{
		TRACE (" + Skipping deadlock encoding, no 'deadlock' atom " \
				"found in spec");
		return;
	}

	// create a new variable for the deadlock atom in the spec
	TRACE (" + Encoding the deadlock constraint: " \
			"dead <=> ~e_1* ^ ... ^ ~e_n*");
	deadlock_var = phi->new_var ();
	SHOW (deadlock_var.to_dimacs (), "d");

	// create clauses for encoding parts C=> and C<=
	clause2[0] = ~deadlock_var;
	clause.push_back (deadlock_var);
	for (n = u.unf.events.next; n; n = n->next)
	{
		e = ls_i (struct event, n, nod);
		if (e->id == 0) continue;

		// if the deadlock atom happens positive, we need part C=>
		if (deadlock_pos)
		{
			clause2[1] = ~var_en (e);
			phi->add_clause (clause2);
		}
		// if the deadlock atom happens negative, we need part C<=
		if (deadlock_neg) clause.push_back (var_en (e));
	}
	if (deadlock_neg) phi->add_clause (clause);
}

void Cunfsat::encode_plain_event_enabled ()
{
	struct event * e;
	struct event * ee;
	struct cond * c;
	sat::Lit p;
	bool lr;
	bool rl;
	int i;
	std::vector<sat::Lit> clause, clause2 (2);
	std::vector<struct event *> list;

	TRACE (" + Encoding 'event enabled' constraints: " \
			"e* <=> ~e ^ /\\_pred(e) ^ /\\_cf(e)");

	/*
	 * if this method is called after both
	 * - encode_plain_trans_enabled
	 * - encode_plain_deadlock
	 * - encode_sym_conflict
	 * then all events for which we need to produce 'event enabled'
	 * constraints will be already in the map event_en_var_map, and all
	 * conditions involved in the definition of e* will also be in
	 * cond_var_map, so all we have to do is to iterate through
	 * event_en_var_map :)
	 */
	for (auto it = event_en_var_map->begin ();
			it != event_en_var_map->end (); ++it)
	{
		SHOW (it->first->ft->name, "s");
		SHOW (it->second.to_dimacs (), "d");
		e = it->first;
		p = it->second;

		// determine whether we need part B=>, part B<=, or both
		lr = e->ft->m == mrk_pos || e->ft->m == mrk_both || deadlock_neg;
		rl = e->ft->m == mrk_neg || e->ft->m == mrk_both || deadlock_pos;

		// prepare clauses for both parts
		clause2[0] = ~p;
		clause.push_back (p);
		if (! e->iscutoff)
		{
			SHOW (e->m, "d");
			clause.push_back (var (e));
			if (lr)
			{
				clause2[1] = ~var (e);
				phi->add_clause (clause2);
			}
		}

		// iterate through all immediate predecessors of e
		list.clear ();
		get_imm_pred (e, list);
		SHOW (list.size(), "d");
		for (auto itt = list.begin(); itt != list.end(); ++itt)
		{
			ee = *itt;
			if (lr)
			{
				clause2[1] = var (ee);
				phi->add_clause (clause2);
			}
			if (rl) clause.push_back (~ var(ee));
		}

		// iterate through all conditions in e's preset
		list.clear ();
		for (i = e->pre.deg - 1; i >= 0; --i)
		{
			c = (struct cond *) e->pre.adj[i];
			if (! cond_var_map->contains (c)) continue;
			if (lr)
			{
				clause2[1] = ~var (c);
				phi->add_clause (clause2);
			}
			if (rl) clause.push_back (var(c));
		}
		if (rl) phi->add_clause (clause);
		clause.clear ();
	}
}

sat::Lit Cunfsat::encode_spec (Spec & s)
{
	switch (s.type)
	{
	case Spec::TRANS :
		return var (s.trans);
	case Spec::DEADLOCK :
		return deadlock_var;
	case Spec::PLACE :
		return var (s.place);
	case Spec::NOT :
		return ~ encode_spec (*s.left);
	default :
		break;
	}

	ASSERT (s.type == Spec::AND || s.type == Spec::OR);
	sat::Lit p = phi->new_var ();
	sat::Lit q = encode_spec (*s.left);
	sat::Lit r = encode_spec (*s.right);
#ifdef VERB_LEVEL_TRACE
	std::string str;
	s.to_str (str);
	TRACE ("Variable %d encodes expr %s", p.to_dimacs (), str.c_str ());
#endif

	if (s.type == Spec::AND)
	{
		// p <=> q ^ r
		phi->add_clause (~p, q);
		phi->add_clause (~p, r);
		phi->add_clause (~q, ~r, p);
	}
	else
	{
		// p <=> q v r
		phi->add_clause (~p, q, r);
		phi->add_clause (~q, p);
		phi->add_clause (~r, p);
	}
	return p;
}

