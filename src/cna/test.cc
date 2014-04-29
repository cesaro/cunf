
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
		ERROR ("'%s'", e.what ());
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

	PRINT (a, "d");
	PRINT (b, "d");
	int & r = a;;
	r = 7;
	PRINT (a, "d");
	PRINT (b, "d");
	r = b;
	PRINT (a, "d");
	PRINT (b, "d");
	r = 8;
	PRINT (a, "d");
	PRINT (b, "d");
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

	PRINT (h (0), "lx");
	PRINT (val = h (&e1), "lx");
	PRINT (h (&e1), "lx");
	PRINT (h (&e2), "lx");
	PRINT (&e1, "p");
	PRINT (&e2, "p");
}

void test ()
{
	//test1 ();
	//test2 ();
	test3 ();
}

} // namespace cna
