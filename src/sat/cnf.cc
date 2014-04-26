
#include <vector>
#include <stdio.h>
#include <stdint.h>

#include "util/config.h"
#include "util/debug.h"
#include "sat/cnf.hh"

namespace sat {


void
Cnf::amo_2tree (std::vector<Lit> & l)
{
	std::vector<Lit> c(2), laux;

	// while there is at least three, build one 'layer' of the tree
	while (l.size () >= 3) {
		for (auto i = l.size () - 1; i >= 2; i -= 2)
		{
			c[0] = ~l[i];
			c[1] = ~l[i - 1];
			add_clause (c);

			c[0] = new_var ();
			add_clause (c);

			c[1] = ~l[i];
			add_clause (c);

			laux.push_back (c[0]);
		}
		l = laux;
		laux.clear ();
	}

	// if zero or one remains, we are done
	if (l.size () <= 1) return;

	// if two remain, either one or the other, but not both
	c[0] = ~l[0];
	c[1] = ~l[1];
	add_clause (c);
}

} // namespace sat

