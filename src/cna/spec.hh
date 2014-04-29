
#ifndef _CNA_SPEC_HH_
#define _CNA_SPEC_HH_

#include <string>
#include <stdio.h>

namespace cna {

class Spec
{
public:
	typedef enum {PLACE, TRANS, DEADLOCK, OR, AND, NOT} type_t;

	type_t type;
	Spec * l;
	Spec * r;
	int trans;
	int place;

	Spec (int pl);
	Spec (type_t t, Spec * left, Spec * right);
	void str (std::string &s);
	~Spec (void);
};

Spec * spec_parse (FILE * f, const std::string & filename);

}

#endif
