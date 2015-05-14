
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
	std::vector<Spec *> spec_buff;

	// open the file if necessary
	if (! f) f = fopen (filename, "r");
	if (! f)
	{
		std::string s = fmt ("%s: %s", filename, strerror (errno));
		throw std::runtime_error (s);
	}
	
	// parse the file
	INFO ("Parsing '%s' ...", filename);
	__cna_filename = filename;
	__cna_specv = &spec_buff;
	__cna_in = f;
	__cna_restart (f);
	__cna_parse ();
	INFO ("Found %d properties", spec_buff.size ());

	specs.resize (spec_buff.size ());
	for (size_t i = 0; i < spec_buff.size(); ++i)
	{
#ifdef VERB_LEVEL_DEBUG
		std::string s;
		spec_buff[i]->to_str (s);
		DEBUG ("Property #%d : %s", i, s.c_str ());
#endif

		specs[i].spec = spec_buff[i];
		specs[i].solved = false;
	}
}

void Speccheck::load_spec_deadlocks ()
{
	// default constructor for Spec is a deadlock specification
	specs.resize (1);
	specs[0].spec = new Spec ();
	specs[0].solved = false;
}

void Speccheck::verify (bool authoritative)
{
	for (size_t i = 0; i < specs.size(); ++i)
	{
		if (! specs[i].solved)
		{
			do_verification (i, authoritative);
		}
	}
}

void Speccheck::print_results ()
{
	std::string s;

	for (size_t i = 0; i < specs.size(); ++i)
	{
		specs[i].spec->to_str (s);

		if (i) PRINT ("");
		PRINT ("Property : #%d", i);
		PRINT ("NNF      : %s", s.c_str ());
		if (specs[i].solved)
		{
			ASSERT (specs[i].result == sat::Cnf::SAT or
					specs[i].result == sat::Cnf::UNSAT);

			bool res = specs[i].result == sat::Cnf::SAT;
			PRINT ("Result   : %s", res ? "SAT" : "UNSAT");
			PRINT ("Model    : %s",
					res ? "(not implemented, but run with -vv ;)" : "n/a");
#ifdef VERB_LEVEL_TRACE
			if (verb_trace && res) {
				TRACE ("Here are the events in the model:");
				TRACE (" = Begin of configuration = ");
				for (auto it = specs[i].model->begin ();
						it != specs[i].model->end (); ++it) db_e (*it);
				TRACE (" = End of configuration = ");
			}
#endif
		}
		else
		{
			s = "UNKNOWN";
			if (specs[i].errmsg.length ()) s += " (" + specs[i].errmsg + ")";
			PRINT ("Result   : %s", s.c_str ());
		}
	}
}

void Speccheck::do_verification (int i, bool authoritative)
{
	INFO ("Verifying property #%d, authoritative=%d", i, authoritative);
	ASSERT (! specs[i].solved);

	// build the encoding and solve it
	specs[i].result = sat::Cnf::UNK;
	specs[i].errmsg.clear ();
	try
	{
		Cunfsat enc (*specs[i].spec);
		enc.encode ();
		specs[i].result = enc.solve ();
#ifdef VERB_LEVEL_TRACE
		if (verb_trace && specs[i].result == sat::Cnf::SAT) {
			specs[i].model = & enc.counterexample ();
		}
#endif
	}
	catch (Minisat::OutOfMemoryException & e)
	{
		// FIXME this should be wrapped by sat::Cnf...
		specs[i].errmsg = "Minisat went out of memory";
	}
	catch (std::exception & e)
	{
		specs[i].errmsg = fmt ("%s", e.what ());
	}
	catch (...)
	{
		specs[i].errmsg = "Runtime error ocurred, that's all I know...";
	}

	/* if we got an error, then errmsg has something and property remains
	 * unsolved */
	if (specs[i].errmsg.length ()) {
		specs[i].result = sat::Cnf::UNK;
		return;
	}

	/* if it was SAT or this is the authoritative verification run, the property
	 * is now solved */
	if (authoritative or specs[i].result == sat::Cnf::SAT)
	{
		specs[i].solved = true;
	}
}

Speccheck::~Speccheck ()
{
	for (auto it = specs.begin(); it != specs.end(); ++it) delete it->spec;
}

