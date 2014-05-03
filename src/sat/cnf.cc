
#include <vector>
#include <stdio.h>
#include <stdint.h>

#include "util/config.h"
#include "util/misc.h"
#include "sat/cnf.hh"

namespace sat {

void Cnf::amo_2tree (std::vector<Lit> & l)
{
	std::vector<Lit> c(2), laux;
	std::vector<Lit>::size_type i;

	// while there is at least three, build one 'layer' of the tree
	while (l.size () >= 3) {
		if (l.size () & 1)
		{
			laux.push_back (l[0]);
			i = 1;
		} else
		{
			i = 0;
		}
		for (; i < l.size (); i += 2)
		{
			c[0] = ~l[i];
			c[1] = ~l[i + 1];
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

void Cnf::add_clause (Lit p)
{
	std::vector<Lit> c (1, p);
	add_clause (c);
}

void Cnf::add_clause (Lit p, Lit q)
{
	std::vector<Lit> c (1, p);
	c.push_back (q);
	add_clause (c);
}

void Cnf::add_clause (Lit p, Lit q, Lit r)
{
	std::vector<Lit> c (1, p);
	c.push_back (q);
	c.push_back (r);
	add_clause (c);
}


} // namespace sat

