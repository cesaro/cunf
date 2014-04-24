#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cassert>

#include "spec_lexer.h"

std::string spec_filename ("input");



#include <stdarg.h>  // for va_start, etc
#include <memory>    // for std::unique_ptr

std::string fmt (const std::string fmt_str, ...) {
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


class Spec
{
public:
	typedef enum {LEAVE, OR, AND, NOT} type_t;

	type_t type;
	Spec * l;
	Spec * r;
	int trans;
	int place;

	Spec (int pl);
	Spec (type_t t, Spec * left, Spec * right);
	void str (std::string &s);
	~Spec (void);
};

Spec::Spec (int pl)
{
	type = LEAVE;
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
	case LEAVE :
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

int
main (const int argc, const char ** argv)
{

	int ret;
	Spec::type_t x;

	x = Spec::OR;

	printf ("%d\n", x);

	while (1) {
		ret = yylex ();
		if (ret == LEND) break;
		// printf ("ret is %d\n", ret);
	}

	std::string s;
	Spec * a = new Spec (Spec::NOT, new Spec (10), 0);
	Spec * b = new Spec (Spec::AND, a, new Spec (20));
	b->str (s);
	std::cout << s << std::endl;
	delete b;
}

