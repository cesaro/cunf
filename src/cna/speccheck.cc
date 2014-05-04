
#include <cstdio>
#include <vector>
#include <string>
#include <stdexcept>

#include "cna/spec_intrnl.hh"
#include "cna/spec.hh"
#include "cna/cunfsat.hh"
#include "cunf/debug.h"

#include "cna/speccheck.hh"

using namespace cna;

/* for communication with the parser */
const char * __cna_filename = 0;
std::vector<Spec *> * __cna_specv = 0;

void Speccheck::load_spec (const char * filename, FILE * f)
{

	// open the file if necessary
	if (! f) f = fopen (filename, "r");
	if (! f)
	{
		std::string s = fmt ("%s: %s", filename, strerror (errno));
		throw std::runtime_error (s);
	}
	
	// clear the state, if necessary
	spec.clear ();

	// parse the file
	INFO ("Parsing '%s' ...", filename);
	__cna_filename = filename;
	__cna_specv = &spec;
	__cna_in = f;
	__cna_restart (f);
	__cna_parse ();
	INFO ("Found %d properties", spec.size ());

#ifdef VERB_LEVEL_DEBUG
	for (size_t i = 0; i < spec.size(); ++i)
	{
		std::string s;
		spec[i]->to_str (s);
		DEBUG ("Property #%d : %s", i, s.c_str ());
	}
#endif
}

void Speccheck::spec_deadlocks ()
{
	// default constructor for Spec is a deadlock specification
	spec.push_back (new Spec ());
}

void Speccheck::verify ()
{
	for (size_t i = 0; i < spec.size(); ++i) verify (spec[i], i);
}

void Speccheck::verify (Spec * s, int i)
{
	INFO ("Checking property #%d", i);

	Cunfsat encoding (*s);
	bool ret;
	std::string nnf;

	encoding.encode ();
	ret = encoding.solve ();
	s->to_str (nnf);

	PRINT ("Property : #%d", i);
	PRINT ("NNF      : %s", nnf.c_str ());
	PRINT ("Result   : %s", ret ? "SAT" : "UNSAT");
	PRINT ("Model    : (not implemented, but run with -vv ;)");

#ifdef VERB_LEVEL_TRACE
	if (verb_trace && ret) {
		TRACE ("Here are the events in the model:");
		std::vector<struct event *> & conf = encoding.counterexample ();
		for (auto it = conf.begin (); it != conf.end (); ++it) db_e (*it);
	}
#endif
}

Speccheck::~Speccheck ()
{
	for (auto it = spec.begin(); it != spec.end(); ++it) delete *it;
}

