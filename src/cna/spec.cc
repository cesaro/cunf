
#include "cna/spec_intrnl.hh"
#include "util/misc.h"

#include "cna/spec.hh"

using namespace cna;

/* for communication between spec_parse and the parser */
const std::string * __cna_filename = 0;
Spec * __cna_ast = 0;



Spec::Spec (int pl)
{
	type = PLACE;
	place = trans = pl;
	l = r = 0;
}

Spec::Spec (type_t t, Spec * left, Spec * right)
{
	ASSERT ((t == NOT && left != 0 && right == 0) ||
			((t == OR || t == AND) &&
			left != 0 && right != 0));
	type = t;;
	l = left;
	r = right;
}

Spec::~Spec (void)
{
	//std::cout << fmt ("destructor this=%p\n", this);
	switch (type) {
	case NOT :
		delete l;
		return;
	case OR :
	case AND :
		delete l;
		delete r;
	default :
		return;
	}
}

void Spec::str (std::string &s)
{
	switch (type) {
	case PLACE :
	case TRANS :
	case DEADLOCK :
		s += fmt ("\"%d\"", place);
		break;
	case OR :
	case AND :
		s += "(";
		l->str (s);
		s += type == OR ? " || " : " && ";
		r->str (s);
		s += ")";
		break;

	case NOT :
		s += "(!";
		l->str (s);
		s += ")";
		break;
	}
}

Spec * cna::spec_parse (FILE * f, const std::string & filename)
{
	__cna_filename = & filename;
	__cna_in = f;
	__cna_restart (f);
	__cna_parse ();

	return __cna_ast;
}

