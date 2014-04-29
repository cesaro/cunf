

#ifndef _SAT_CNF_HH_
#define _SAT_CNF_HH_

#include <unordered_map>
#include <vector>
#include <cstdlib>

namespace sat {

typedef int Var;

class Lit
{
public:
	Lit () : x(0) {} // do not use this !
	Lit (Var v, bool sign = false) : x((v << 1) + (int) sign) {}

	bool sign (void) { return x & 1; }
	Var var (void) { return x >> 1; }
	Lit operator~ () { return Lit (x ^ 1); }

	int to_dimacs () { return (sign () ? -var() - 1 : var () + 1); }
	void from_dimacs (int i) { x = (i < 0 ? ((-i-1) << 1) + 1 : (i-1) << 1); }
		
private:
	Lit (unsigned _x) : x(_x) {};
	unsigned x;
};


class CnfModel
{
public:
	virtual bool operator[] (Var v) const =0;
	virtual bool operator[] (Lit p) const =0;
	// iterator
};


class Cnf
{
public:
	typedef enum {SAT, UNSAT, UNK} result_t;

	virtual ~Cnf () {};

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
public:
	VarMap (Cnf & _f) : map(), f(_f) {};

	Lit operator[] (const T & k)
	{
		auto it = map.find (k);
		if (it != map.end()) return it->second;
		return map[k] = f.new_var ();
	}

private:
	std::unordered_map<T,Lit,Hash> map;
	Cnf & f;
};

}

#endif
