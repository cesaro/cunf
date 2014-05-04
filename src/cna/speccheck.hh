
#ifndef _CNA_SPECCHECK_HH_
#define _CNA_SPECCHECK_HH_

#include <vector>
#include <string>
#include <cstdio>

#include "cna/spec.hh"

namespace cna {

class Speccheck
{
public:
	void load_spec (const char * filename, FILE * f = 0);
	void load_spec (const std::string & filename, FILE * f = 0);
	void spec_deadlocks ();
	void verify ();

	~Speccheck ();

private:
	void verify (Spec * s, int i);

	// internal state
	std::vector<Spec *> spec;
};

inline void load_spec (const std::string & filename, FILE * f = 0)
{
	return load_spec (filename.c_str (), f);
}

void test ();

} // namespace cna

#endif
