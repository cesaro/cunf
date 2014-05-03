
#ifndef _CNA_SPEC_HH_
#define _CNA_SPEC_HH_

#include <string>
#include <stdio.h>

#include "cunf/global.h"

namespace cna {

class Spec
{
public :
	typedef enum {PLACE, TRANS, DEADLOCK, OR, AND, NOT} type_t;

	// type of the node and pointers to the leaves or children nodes
	type_t type;
	struct place * place;
	struct trans * trans;
	class Spec * left;
	class Spec * right;

	// constructors
	Spec (const std::string & filename, FILE * f = 0) : Spec (filename.c_str (), f) {}
	Spec (const char * filename, FILE * f = 0);
	Spec (); // deadlock
	Spec (struct place * p);
	Spec (struct trans * t);
	Spec (type_t t, Spec * l, Spec * r = 0);

	// destructor
	~Spec (void);

	// other methods
	Spec & operator= (const Spec & rhs);
	void str (std::string &s);
	void to_nnf ();

private :
	void copy_from (const Spec & from);
	void push_negations (Spec & s);
};


inline void Spec::to_nnf () { push_negations (*this); }

} // namespace cna

#endif
