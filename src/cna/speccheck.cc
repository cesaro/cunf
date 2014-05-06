
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
	// hack for the MCC'14, make this better
	results.resize (spec.size());
	for (size_t i = 0; i < spec.size(); ++i)
	{
		if (i) PRINT ("");
		do_verification (spec[i], i);
	}
}

void Speccheck::do_verification (Spec * s, int i)
{
	bool ret = false;
	std::string nnf;
	std::string errmsg;
#ifdef VERB_LEVEL_TRACE
	std::vector<struct event *> * conf;
#endif

	INFO ("Verifying property #%d", i);

	try
	{
		// hack
		if (i == 1
				&& s->type == Spec::DEADLOCK
				&& spec[0]->type == Spec::DEADLOCK)
		{
			ret = results[0] == 1;
			if (results[0] == 2) throw std::runtime_error ("same error");
		}
		else
		{
			Cunfsat enc (*s);
			enc.encode ();
			ret = enc.solve ();
#ifdef VERB_LEVEL_TRACE
			if (verb_trace && ret) {
				conf = & enc.counterexample ();
			}
#endif
		}
	}
	catch (std::exception & e)
	{
		errmsg = fmt ("%s", e.what ());
	}
	catch (Minisat::OutOfMemoryException & e)
	{
		// FIXME this should be wrapped by sat::Cnf...
		errmsg = "Minisat went out of memory";
	}
	catch (...)
	{
		errmsg = "Runtime error ocurred, that's all I know...";
	}

	s->to_str (nnf);
	PRINT ("Property : #%d", i);
	PRINT ("NNF      : %s", nnf.c_str ());

	if (errmsg.size ())
	{
		results[i] = 2; // hack
		PRINT ("Result   : UNKNOWN (%s)", errmsg.c_str());
		PRINT ("Model    : n/a");
	}
	else
	{
		results[i] = ret ? 1 : 0; // hack
		PRINT ("Result   : %s", ret ? "SAT" : "UNSAT");
		PRINT ("Model    : %s",
				ret ? "(not implemented, but run with -vv ;)" : "n/a");
#ifdef VERB_LEVEL_TRACE
		if (verb_trace && ret) {
			TRACE ("Here are the events in the model:");
			TRACE (" = Begin of configuration = ");
			for (auto it = conf->begin (); it != conf->end (); ++it) db_e (*it);
			TRACE (" = End of configuration = ");
		}
#endif
	}
}

Speccheck::~Speccheck ()
{
	for (auto it = spec.begin(); it != spec.end(); ++it) delete *it;
}

