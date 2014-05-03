
#include <cerrno>
#include <cstring>
#include <string>
#include <stdexcept>

#include "cna/spec_intrnl.hh"
#include "util/misc.h"
#include "cna/spec.hh"

using namespace cna;

/* for communication between spec_parse and the parser */
const char * __cna_filename = 0;
Spec * __cna_ast = 0;

Spec::Spec (const char * filename, FILE * f)
{
	// open the file if necessary
	SHOW (filename, "s");
	SHOW (f, "p");
	if (! f) f = fopen (filename, "r");
	if (! f)
	{
		std::string s = fmt ("%s: %s", filename, strerror (errno));
		throw std::runtime_error (s);
	}

	// parse the file
	__cna_filename = filename;
	__cna_in = f;
	__cna_restart (f);
	__cna_parse ();

	// move the subformula store in the parsed abstract syntax tree
	SHOW (__cna_ast->type, "d");
	SHOW (__cna_ast->left, "d");
	SHOW (__cna_ast->right, "d");
	copy_from (*__cna_ast);
	SHOW (this->left, "d");
	SHOW (this->right, "d");

	// safely destroy the memory of the root spec
	__cna_ast->type = DEADLOCK;
	delete __cna_ast;
}

Spec::Spec ()
{
	type = DEADLOCK;
}

Spec::Spec (struct place * p)
{
	type = PLACE;
	place = p;
}

Spec::Spec (struct trans * t)
{
	type = TRANS;
	trans = t;
}

Spec::Spec (type_t t, Spec * l, Spec * r)
{
	ASSERT ((t == NOT && l != 0 && r == 0) ||
			((t == OR || t == AND) && l != 0 && r != 0));
	type = t;;
	left = l;
	right = r;
}

Spec::~Spec (void)
{
	//DEBUG ("Spec destructor this=%p\n", this);
	switch (type) {
	case NOT :
		delete left;
		return;
	case OR :
	case AND :
		delete left;
		delete right;
	default :
		break;
	}
}

void Spec::str (std::string &s)
{
	switch (type) {
	case PLACE :
		ASSERT (place);
		s += fmt ("\"%s\"", place->name);
		break;
	case TRANS :
		ASSERT (trans);
		s += fmt ("\"%s\"", trans->name);
		break;
	case DEADLOCK :
		s += "deadlock";
		break;
	case OR :
	case AND :
		ASSERT (left && right);
		s += "(";
		left->str (s);
		s += type == OR ? " || " : " && ";
		right->str (s);
		s += ")";
		break;

	case NOT :
		ASSERT (left);
		s += "(!";
		left->str (s);
		s += ")";
		break;
	}
}

Spec & Spec::operator= (const Spec & rhs)
{
	// this is very adhoc, fix this using the swap copy ctor idiom
	switch (type) {
	case NOT :
		delete left;
		break;
	case OR :
	case AND :
		delete left;
		delete right;
	default :
		break;
	}

	copy_from (rhs);
	return *this;
}

void Spec::copy_from (const Spec & from)
{
	if (this == &from) return;

	type = from.type;
	place = from.place;
	trans = from.trans;
	left = from.left;
	right = from.right;
}

void Spec::push_negations (Spec & s)
{
	Spec * ss;

	std::string s_;
	s.str (s_);
	SHOW (s_.c_str (), "s");

	switch (s.type)
	{
	case PLACE :
	case TRANS :
	case DEADLOCK :
		// if this is a leave, nothing to do
		return;
	case AND :
	case OR :
		// if there is a branch, explore both branches
		push_negations (* s.left);
		push_negations (* s.right);
		return;
	default:
		break;
	}

	// the only remaining case is a negation
	ASSERT (s.type == NOT);
	ASSERT (s.left && !s.right);
	switch (s.left->type)
	{
	case PLACE :
	case TRANS :
	case DEADLOCK :
		// if this was a literal, nothing to do
		return;
	case NOT :
		// if this is a double negation, simplify and continue on same node
		ASSERT (s.left->left);
		ss = s.left;
		s.copy_from (*s.left->left);
		ss->left->type = DEADLOCK; // prevents deleting the entire subformula!
		delete ss;
		push_negations (s);
		return;
	default:
		break;
	}

	// the only remaining case is a negation of disjunction or conjunction
	ASSERT (s.left->type == AND || s.left->type == OR);

	// apply De Morgan's laws: create new node for the right branch, change
	// my type, and update my children
	s.type = s.left->type == AND ? OR : AND;
	s.left->type = NOT;
	s.right = new Spec (NOT, s.left->right);
	s.left->right = 0;

	// continue pushing negations on both branches
	push_negations (*s.left);
	push_negations (*s.right);
}

