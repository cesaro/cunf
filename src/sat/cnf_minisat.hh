
#ifndef _SAT_CNF_MINISAT_HH_
#define _SAT_CNF_MINISAT_HH_

#include "sat/cnf.hh"

//#define __STDC_LIMIT_MACROS // for the minisat source tree
//#define __STDC_FORMAT_MACROS
#include "minisat/core/Solver.h"

namespace sat {

class Msat;

class MsatModel : public CnfModel
{
public :
	MsatModel (const Msat & _f) : f(_f) {}

	virtual bool
	operator[] (Lit p) const
	{
		// f.s.
		return p.sign ();
	}

private :
	const Msat & f;
};

class Msat : public Cnf
{
public :
	Lit new_var (void);
	void add_clause (std::vector<Lit> & clause);
	result_t solve (void);
	const CnfModel & get_model (void);

private :
	int a;
	Minisat::Solver s;
};

} // namespace sat

#endif
