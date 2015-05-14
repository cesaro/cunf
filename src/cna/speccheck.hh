
#ifndef _CNA_SPECCHECK_HH_
#define _CNA_SPECCHECK_HH_

#include <vector>
#include <string>
#include <cstdio>

#include "cna/spec.hh"
#include "cna/cunfsat.hh"

namespace cna {

class Speccheck
{
public:
	void load_spec (const char * filename, FILE * f = 0);
	void load_spec (const std::string & filename, FILE * f = 0);
	void load_spec_deadlocks ();
	void verify (bool authoritative = false);
	void print_results ();

	~Speccheck ();

private:
	void do_verification (int i, bool authoritative);

	// internal state
	struct spec_container
	{
		Spec * spec;
		Cunfsat::result_t result;
		bool solved;
		std::string errmsg;
		std::vector<struct event *> * model;
	};
	std::vector<struct spec_container> specs;
	std::vector<int> results;
};

inline void Speccheck::load_spec (const std::string & filename, FILE * f)
{
	return load_spec (filename.c_str (), f);
}

void test ();

} // namespace cna

#endif
