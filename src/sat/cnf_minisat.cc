
#include "sat/cnf_minisat.hh"

namespace sat {

void Msat::add_clause (std::vector<Lit> & c)
{
	Minisat::vec<Minisat::Lit> mc;

	for (auto it = c.begin (); it != c.end (); ++it)
	{
		mc.push (Minisat::mkLit (it->var (), it->sign ()));
	}
	s.addClause_ (mc);
}

Msat::result_t Msat::solve ()
{
	Minisat::lbool ret;
	Minisat::vec<Minisat::Lit> dummy;

	if (! s.simplify ()) return UNSAT;

        ret = s.solveLimited (dummy);
	if (ret == Minisat::l_True) return SAT;
	if (ret == Minisat::l_False) return UNSAT;
	return UNK;
}

} // namespace sat

