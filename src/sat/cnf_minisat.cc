
#include "sat/cnf_minisat.hh"

namespace sat {


Lit
Msat::new_var (void)
{
	return Lit (s.newVar ());
}

void
Msat::add_clause (std::vector<Lit> & c)
{
	Minisat::vec<Minisat::Lit> mc;

	for (auto it = c.begin (); it != c.end (); ++it)
	{
		mc.push (Minisat::mkLit (it->var (), it->sign ()));
	}
	s.addClause_ (mc);
}

Msat::result_t
Msat::solve (void)
{
	Minisat::lbool ret;
	Minisat::vec<Minisat::Lit> dummy;

	if (! s.simplify ()) return UNSAT;

        ret = s.solveLimited (dummy);
	if (ret == Minisat::l_True) return SAT;
	if (ret == Minisat::l_False) return UNSAT;
	return UNK;
}

const CnfModel &
Msat::get_model (void)
{
	return MsatModel (*this);
}


} // namespace sat

