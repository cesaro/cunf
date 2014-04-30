
#include "sat/cnf_minisat.hh"

namespace sat {

void Msat::add_clause (std::vector<Lit> & c)
{
	Minisat::vec<Minisat::Lit> mc;

	DEBUG_ ("( ");
	for (auto it = c.begin (); it != c.end (); ++it)
	{
		DEBUG_ ("%d ", it->to_dimacs ());
		mc.push (Minisat::mkLit (it->var (), it->sign ()));
	}
	DEBUG (")");
	s.addClause_ (mc);
}

Msat::result_t Msat::solve ()
{
	Minisat::lbool ret;
	Minisat::vec<Minisat::Lit> dummy;

	SHOW (s.okay (), "d");
	if (! s.simplify ())
	{
		TRACE ("Query solved by unit propagation");
		return UNSAT;
	}

	ret = s.solveLimited (dummy);
	if (ret == Minisat::l_True) return SAT;
	if (ret == Minisat::l_False) return UNSAT;
	return UNK;
}

} // namespace sat

