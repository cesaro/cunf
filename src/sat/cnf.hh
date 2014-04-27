

#ifndef _SAT_CNF_HH_
#define _SAT_CNF_HH_

#include <unordered_map>
#include <vector>

namespace sat {

typedef int Var;

class Lit
{
public:
	Lit () : v(0) {} // do not use this !
	Lit (Var x, bool sign = false) : v((x << 1) + (int) sign) {}

	bool sign (void) { return v & 1; }
	Var var (void) { return v >> 1; }
	Lit operator~ () { return Lit (v ^ 1); }

private:
	Lit (unsigned _v) : v(_v) {};
	unsigned v;
};


class CnfModel
{
public:
	virtual bool operator[] (Lit p) const =0;
	// iterator
};


class Cnf
{
public:
	typedef enum {SAT, UNSAT, UNK} result_t;

	virtual Var no_vars (void) =0;
	virtual Lit new_var (void) =0;
	virtual void add_clause (std::vector<Lit> & clause) =0;
	virtual result_t solve (void) = 0;
	virtual CnfModel & get_model (void) =0;

	void amo_2tree (std::vector<Lit> & lits);
};


// type T needs to define operator==
template<class T, class Hash = std::hash<T>>
class VarMap
{
	VarMap (Cnf & _f) : map(), f(_f) {};

	Lit
	operator[] (const T & k)
	{
		auto it = map.find (k);
		if (it != map.end()) return *it;
		Lit p = f.new_var ();
		return map[k] = p;
	}

private:
	std::unordered_map<T,Lit,Hash> map;
	Cnf & f;
};

}

#endif
