#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cassert>

#include <err.h>

#include "spec.hh"
#include "debug.h"

int
main (const int argc, const char ** argv)
{
	std::string path;
	cna::Spec * spec;
	FILE * f;

	path = "input";
	f = fopen (path.c_str (), "r");
	if (f == 0) err (1, "%s", path.c_str ());

	spec = cna::spec_parse (f, path);
	fclose (f);

	std::string s;
	spec->str (s);
	TRACE (s.c_str(), "s");

	return 0;
}

