
#ifndef _CNA_CUNFSAT_HH_
#define _CNA_CUNFSAT_HH_

#include <cstdio>
#include <unordered_set>

#include "sat/cnf.hh"
#include "sat/cnf_minisat.hh"
#include "cna/spec.hh"
#include "cunf/global.h"

namespace cna {

class Cunfsat
{
public:
	Cunfsat (Spec & s);
	~Cunfsat ();

	void encode ();
	bool solve ();
	std::vector<struct event *> & counterexample ();

#if 0
	struct {
		bool deadlock : 1;
	} options;
#endif

private:
	// encoding for CONTEXTUAL nets
	void encode_ctx ();
	void encode_ctx_config ();
	void encode_ctx_deadlock ();
	void encode_ctx_causality ();
	void encode_ctx_sym_conflict ();
	void encode_ctx_asym_conflict ();
	void encode_ctx_all_disabled ();
	void encode_ctx_mrk_cond (struct cond * c);

	// optimized encoding for PLAIN nets
	void encode_plain ();
	void encode_plain_trans_enabled ();
	void encode_plain_event_enabled ();
	void encode_plain_deadlock ();
	void encode_plain_causality ();
	void encode_plain_sym_conflict ();
	void encode_plain_spec ();

	// auxiliar methods
	bool is_stubborn (struct event * e);
	void get_imm_pred (struct event * e, std::vector<struct event *> & l);
	sat::Lit var (struct event * e);
	sat::Lit var_en (struct event * e);
	sat::Lit var (struct place * p);
	sat::Lit var (struct trans * t);

	// transforming and exploring the specification
	void mark_atoms (Spec & s);

	// asserting this implementation is sound
	void assert_is_config (int m);
	void assert_disables_all (int m);
	void assert_is_deadlock (int m);
	void assert_get_marking (int m, std::unordered_set<struct place *> & mrk);
	void assert_disables_t
			(std::unordered_set<struct place *> & mrk, struct trans * t);

	// internal state: the spec, formula, variable mappings, and other mess
	Spec & spec;
	sat::Cnf * phi;
	sat::Msat * __phi_msat;
	sat::VarMap<struct event*> * event_var_map;
	sat::VarMap<struct event*> * event_en_var_map;
	sat::VarMap<struct place*> * place_var_map;
	sat::VarMap<struct trans*> * trans_var_map;
	std::vector<struct event*> violating_run;	
	std::vector<struct trans*> atoms;
	sat::Lit deadlock_var;

	// internal state for the routines of the plain encoding
	int mrk_pos;
	int mrk_neg;
	int mrk_both;
	bool deadlock_pos;
	bool deadlock_neg;
};

inline sat::Lit Cunfsat::var (struct event * e) {
	sat::Lit p = (*event_var_map)[e];
	DEBUG ("Event e%d:%s is variable %d", e->id, e->ft->name, p.to_dimacs ());
	return p;
}
inline sat::Lit Cunfsat::var_en (struct event * e) {
	sat::Lit p = (*event_en_var_map)[e];
	DEBUG ("Event e%d:%s-enabled is variable %d", e->id, e->ft->name, p.to_dimacs ());
	return p;
}
inline sat::Lit Cunfsat::var (struct place * p) {
	sat::Lit q = (*place_var_map)[p];
	DEBUG ("Place %s is variable %d", p->name, q.to_dimacs ());
	return q;
}
inline sat::Lit Cunfsat::var (struct trans * t) {
	sat::Lit q = (*trans_var_map)[t];
	DEBUG ("Trans %s is variable %d", t->name, q.to_dimacs ());
	return q;
}

void test ();

} // namespace cna
#endif
