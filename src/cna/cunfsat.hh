
#ifndef _CNA_CUNFSAT_HH_
#define _CNA_CUNFSAT_HH_

#include <cstdio>
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
	void encode_config ();
	void encode_deadlock ();
	void encode_causality ();
	void encode_sym_conflict ();
	void encode_asym_conflict ();
	void encode_all_disabled ();
	void encode_mrk_cond (struct cond * c);

	bool is_stubborn (struct event * e);
	void get_imm_pred (struct event * e, std::vector<struct event *> & l);
	sat::Lit var (struct event * e) { return (*event_var_map)[e]; }
	sat::Lit var (struct place * p) { return (*place_var_map)[p]; }

	const Spec * spec;
	std::vector<struct event *> violating_run;	

	sat::Cnf * phi;
	sat::Msat * __phi_msat;

	// std::hash() for pointers is just the pointer :)
	sat::VarMap<struct event*> * event_var_map;
	sat::VarMap<struct place*> * place_var_map;
};

} // namespace cna
#endif
