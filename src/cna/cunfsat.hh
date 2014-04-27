
#ifndef _CNA_CUNFSAT_HH_
#define _CNA_CUNFSAT_HH_

#include "sat/cnf.hh"
#include "cna/spec.hh"

namespace cna {

class Cunfsat
{
public:
	Cunfsat (const Spec & _spec, sat::Cnf & _phi) :
			spec (_spec),
			phi (_phi),
			event_var_map (phi),
			place_var_map (phi)
	{};

	void encode ();

	struct {
		bool deadlock : 1;
	} options;

private:
	void encode_config ();
	void encode_causality ();
	void encode_sym_conflict ();
	void encode_asym_conflict ();
	void encode_deadlock ();
	void encoding_mrk_cond (struct cond * c);

	void get_imm_pred (struct event * e, std::vector<struct event *> & l);

	sat::Lit var (struct event * e) { return event_var_map[e]; }
	sat::Lit var (struct place * p) { return place_var_map[p]; }
	bool is_stubborn (struct event * e);

	const Spec & spec;
	sat::Cnf & phi;

	// FIXME ensure that std::hash(e) is the pointer!
	sat::VarMap<struct event*> event_var_map;
	sat::VarMap<struct place*> place_var_map;
};

} // namespace cna
#endif
