
#include <string>
#include "cna/cunfsat.hh"
#include "util/misc.h"
#include "cunf/global.h"

namespace cna {

/*
 * parsing some file
 */
void test1 ()
{
	DEBUG ("Loading file 'input'");
	Spec s ("input");

	std::string str;
	s.str (str);
	DEBUG2 ("Parsed specification: '%s'", str.c_str ());
	SHOW (s.type, "d");

	s.to_nnf ();
	str.clear ();
	s.str (str);
	DEBUG2 ("Now in in NNF: '%s'", str.c_str ());

	Cunfsat enc (s);
	enc.encode ();
}

/*
 * understanding references
 */
void test2 ()
{
	int a, b;

	a = 1;
	b = 2;

	SHOW (a, "d");
	SHOW (b, "d");
	int & r = a;;
	r = 7;
	SHOW (a, "d");
	SHOW (b, "d");
	r = b;
	SHOW (a, "d");
	SHOW (b, "d");
	r = 8;
	SHOW (a, "d");
	SHOW (b, "d");
}

/*
 * ensure that std::hash(ptr) is just ptr!
 */
void test3 ()
{
	std::hash<struct event *> h;
	// std::hash<struct event> hp; <-- std::hash is not specialized for this
	size_t val;

	struct event e1;
	struct event e2;

	SHOW (h (0), "lx");
	SHOW (val = h (&e1), "lx");
	SHOW (h (&e1), "lx");
	SHOW (h (&e2), "lx");
	SHOW (&e1, "p");
	SHOW (&e2, "p");
}

void test ()
{
	test1 ();
	//test2 ();
	//test3 ();
}

} // namespace cna
