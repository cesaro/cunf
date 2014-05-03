
#ifndef _CNA_SPECCHECK_HH_
#define _CNA_SPECCHECK_HH_

#include <vector>
#include <string>

#include "cna/spec.hh"

namespace cna {

class Speccheck
{
public:
	void load_spec (const std::string & filename);

private:
	std::vector<Spec *> spec;
};

} // namespace cna

#endif
