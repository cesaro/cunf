#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cassert>
#include <memory>
#include <stdarg.h>

#include "spec.hh"
#include "spec_intrnl.hh"

using namespace cna;

/* for communication between spec_parse and the parser */
std::string __cna_filename;
Spec * __cna_ast = 0;


std::string cna::fmt (const std::string fmt_str, ...) {
	/* reserve 2 times as much as the length of the fmt_str */
	int n = fmt_str.size() * 2;
	int final_n;
	std::string str;
	std::unique_ptr<char[]> formatted;

	va_list ap;
	while(1) {
		formatted.reset (new char[n]);
		strcpy (&formatted[0], fmt_str.c_str());
		va_start(ap, fmt_str);
		final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
		va_end(ap);
		if (final_n < 0 || final_n >= n)
			n += abs(final_n - n + 1);
		else
			break;
	}
	return std::string(formatted.get());
}


Spec::Spec (int pl)
{
	type = PLACE;
	place = trans = pl;
	l = r = 0;
}

Spec::Spec (type_t t, Spec * left, Spec * right)
{
	assert ((t == NOT && left != 0 && right == 0) ||
			((t == OR || t == AND) &&
			left != 0 && right != 0));
	type = t;;
	l = left;
	r = right;
}

Spec::~Spec (void)
{
	std::cout << fmt ("destructor this=%p\n", this);
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

Spec *
cna::spec_parse (FILE * f, const std::string & filename)
{
	__cna_filename = filename;
	__cna_in = f;
	__cna_restart (f);
	__cna_parse ();

	return __cna_ast;
}

