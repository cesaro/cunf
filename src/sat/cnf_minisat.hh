
#ifndef _SAT_CNF_MINISAT_HH_
#define _SAT_CNF_MINISAT_HH_

#include "sat/cnf.hh"
#include "util/misc.h"

#include "minisat/core/Solver.h"

namespace sat {

class Msat;

class MsatModel : public CnfModel
{
public:
	bool operator[] (Lit p) const;
	bool operator[] (Var v) const;

protected:
	MsatModel (const Msat & _f) : f(_f) {}
	const Msat & f;

	friend class Msat;
};


class Msat : public Cnf
{
public:
	Msat () : m (*this) {};
	~Msat () {};
	Var no_vars ();
	Var no_clauses ();
	Lit new_var ();
	void add_clause (std::vector<Lit> & clause);
	result_t solve ();
	CnfModel & get_model ();

protected:
	Minisat::Solver s;
	MsatModel m;

	friend class MsatModel;
};


inline Var Msat::no_vars () { return s.nVars (); }
inline Var Msat::no_clauses () { return s.nClauses (); }
inline Lit Msat::new_var () { return Lit (s.newVar ()); }
inline CnfModel & Msat::get_model () { return m; }


inline bool MsatModel::operator[] (Lit p) const
{
	return operator[] (p.var ());
}
inline bool MsatModel::operator[] (Var v) const
{
	ASSERT (v >= 0);
	ASSERT (v < f.s.nVars ());
	if (f.s.model[v] != Minisat::l_True && f.s.model[v] != Minisat::l_False)
		DEBUG ("v %d is UNDEF", v);

	return f.s.model[v] == Minisat::l_True;
}

} // namespace sat

#endif
