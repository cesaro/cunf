
#include <cerrno>
#include <cstring>
#include <string>
#include <stdexcept>

#include "util/misc.h"
#include "cna/spec.hh"

using namespace cna;

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

void Spec::to_str (std::string &s)
{
	bool sw;
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
		left->to_str (s);
		s += type == OR ? " || " : " && ";
		right->to_str (s);
		s += ")";
		break;

	case NOT :
		ASSERT (left);
		sw = left->type == OR || left->type == AND;
		if (sw) s += "(";
		s += "!";
		left->to_str (s);
		if (sw) s += ")";
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

#ifdef VERB_LEVEL_DEBUG
	std::string s_;
	s.to_str (s_);
	SHOW (s_.c_str (), "s");
#endif

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

