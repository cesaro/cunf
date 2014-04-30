
#include <string>
#include "cna/cunfsat.hh"
#include "util/misc.h"
#include "cunf/global.h"

namespace cna {

/*
 * testing exceptions
 */
void test1 ()
{
	{
		Cunfsat a;
		std::string s ("input");
		a.load_spec (s);
	}

	try
	{
		Cunfsat a;
		std::string s ("inputtt");
		a.load_spec (s);
	}
	catch (std::exception & e)
	{
		PRINT ("'%s'", e.what ());
		throw;
	}
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
	//test1 ();
	//test2 ();
	test3 ();
}

} // namespace cna
