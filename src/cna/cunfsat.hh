
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
	Cunfsat ();
	~Cunfsat ();

	void load_spec (const std::string & filename);
	void encode ();
	bool solve ();
	std::vector<struct event *> & counterexample ();

#if 0
	struct {
		bool deadlock : 1;
	} options;
#endif

private:
	// bulding the different fragments of the encoding
	void encode_config ();
	void encode_deadlock ();
	void encode_causality ();
	void encode_sym_conflict ();
	void encode_asym_conflict ();
	void encode_all_disabled ();
	void encode_mrk_cond (struct cond * c);

	// auxiliar methods
	bool is_stubborn (struct event * e);
	void get_imm_pred (struct event * e, std::vector<struct event *> & l);
	sat::Lit var (struct event * e);
	sat::Lit var (struct place * p);

	// asserting this implementation is sound
	void assert_is_config (int m);
	void assert_disables_all (int m);
	void assert_is_deadlock (int m);
	void assert_get_marking (int m, std::unordered_set<struct place *> & mrk);
	void assert_disables_t
			(std::unordered_set<struct place *> & mrk, struct trans * t);

	// internal state: the spec and a formula
	const Spec * spec;
	std::vector<struct event *> violating_run;	
	sat::Cnf * phi;
	sat::Msat * __phi_msat;
	sat::VarMap<struct event*> * event_var_map;
	sat::VarMap<struct place*> * place_var_map;
};

inline sat::Lit Cunfsat::var (struct event * e) {
	sat::Lit p = (*event_var_map)[e];
	DEBUG ("Event e%d:%s is variable %d", e->id, e->ft->name, p.to_dimacs ());
	return p;
}
inline sat::Lit Cunfsat::var (struct place * p) {
	sat::Lit q = (*place_var_map)[p];
	DEBUG ("Place %s is variable %d", p->name, q.to_dimacs ());
	return q;
}

} // namespace cna
#endif
