
#include <vector>
#include <set>
#include <queue>
#include <string>
#include <algorithm>
#include <iostream>

#include "util/config.h"
#include "util/debug.h"
#include "net/net.hh"

std::ostream & operator<< (std::ostream & os, const Trans & t)
{
	os << "name '" << t.name;
	os << "' pre " << t.pre;
	os << " cont " << t.cont;
	return os << " post " << t.post;
}

std::ostream & operator<< (std::ostream & os, const Place & p)
{
	return os << "'" << p.name << "'";
}

std::ostream & operator<< (std::ostream & os, const Marking & m)
{
	std::vector<unsigned long>::size_type i;

	os << "{";
	for (i = 0; i < m.mrk.size (); i++) {
		if (m.mrk[i] == 0) continue;
		os << " '" << m.net->places[i]->name << "'='" << m.mrk[i];
	}
	return os << " }";
}

template<typename T>
std::ostream & operator<< (std::ostream & os, const std::vector<T> & v)
{
	os << "[";
	if (v.empty ()) return os << "]";
	auto p = v.begin ();
	for (; p + 1 != v.end (); p++) {
		os << *p << ", ";
	}
	return os << *p << "]";
}

template<typename T>
std::ostream & operator<< (std::ostream & os, const std::vector<T *> & v)
{
	os << "[";
	if (v.empty ()) return os << "]";
	auto p = v.begin ();
	for (; p + 1 != v.end (); p++) {
		os << **p << ", ";
	}
	return os << **p << "]";
}


Trans::Trans (const std::string & name,
			std::vector<Place *>::size_type idx) :
	id (idx),
	name (name),
	m (0)
{
}

Trans::~Trans ()
{
} // member variables are automatically destructed here

void Trans::pre_add (Place & p, bool call_oppst)
{
	pre.push_back (&p);
	if (call_oppst) p.post_add (*this, false);
}

void Trans::post_add (Place & p, bool call_oppst)
{
	post.push_back (&p);
	if (call_oppst) p.pre_add (*this, false);
}

void Trans::cont_add (Place & p, bool call_oppst)
{
	cont.push_back (&p);
	if (call_oppst) p.cont_add (*this, false);
}

void Trans::pre_rem (Place & p, bool call_oppst)
{
	std::vector<Place *>::iterator it =
		std::find (pre.begin (), pre.end (), &p);
	ASSERT (it != pre.end ());
	// FIXME throw exception
	if (it != pre.end () - 1) *it = pre.back ();
	pre.pop_back ();
	if (call_oppst) p.post_rem (*this, false);
}

void Trans::post_rem (Place & p, bool call_oppst)
{
	std::vector<Place *>::iterator it = post.begin ();
	for (; it != post.end (); it++) {
		if(*it != &p) continue;
		if (it != post.end () - 1) *it = post.back ();
		post.pop_back ();
		if (call_oppst) p.pre_rem (*this, false);
		return;
	}
	ASSERT (false);
	// FIXME throw exception
}

void Trans::cont_rem (Place & p, bool call_oppst)
{
	std::vector<Place *>::iterator it = cont.begin ();
	for (; it != cont.end (); it++) {
		if(*it != &p) continue;
		if (it != cont.end () - 1) *it = cont.back ();
		cont.pop_back ();
		if (call_oppst) p.cont_rem (*this, false);
		return;
	}
	ASSERT (false);
	// FIXME throw exception
}

Place::Place (const std::string & name,
			std::vector<Place *>::size_type idx) :
	id (idx),
	name (name),
	m (0)
{
}

Place::~Place ()
{
}

void Place::pre_add (Trans & t, bool call_oppst)
{
	pre.push_back (&t);
	if (call_oppst) t.post_add (*this, false);
}

void Place::post_add (Trans & t, bool call_oppst)
{
	post.push_back (&t);
	if (call_oppst) t.pre_add (*this, false);
}

void Place::cont_add (Trans & t, bool call_oppst)
{
	cont.push_back (&t);
	if (call_oppst) t.cont_add (*this, false);
}

void Place::pre_rem (Trans & t, bool call_oppst)
{
	auto it = find (pre.begin (), pre.end (), &t);
	ASSERT (it != pre.end ());
	// FIXME throw exception
	if (it != pre.end () - 1) *it = pre.back ();
	pre.pop_back ();
	if (call_oppst) t.post_rem (*this, false);
}

void Place::post_rem (Trans & t, bool call_oppst)
{
	std::vector<Trans *>::iterator it =
		find (post.begin (), post.end (), &t);
	ASSERT (it != post.end ());
	// FIXME throw exception
	if (it != post.end () - 1) *it = post.back ();
	post.pop_back ();
	if (call_oppst) t.pre_rem (*this, false);
}

void Place::cont_rem (Trans & t, bool call_oppst)
{
	std::vector<Trans *>::iterator it =
		find (cont.begin (), cont.end (), &t);
	ASSERT (it != cont.end ());
	// FIXME throw exception
	if (it != cont.end () - 1) *it = cont.back ();
	cont.pop_back ();
	if (call_oppst) t.cont_rem (*this, false);
}

Net::Net (void) :
	m0 (*this)
{
}

Place & Net::place_add (const std::string name, unsigned long m0)
{
	Place * p = new Place (name, places.size ());
	places.push_back (p);
	this->m0.resize ();
	this->m0[*p] = m0;
	return *p;
}

Trans & Net::trans_add (const std::string name)
{
	Trans * t = new Trans (name, trans.size ());
	trans.push_back (t);
	return *t;
}

bool Net::is_reachable (const Marking & target) const
{
	struct __mrk_cmp {
		bool operator () (const Marking * m1, const Marking * m2) {
			return *m1 < *m2;
		}
	};
	std::set<Marking *, __mrk_cmp> reach;
	std::queue<Marking *> work;
	std::pair<std::set<Marking *>::iterator, bool> ret;
	std::vector<Trans *> ena;
	Marking * m;
	bool result;

	m = new Marking (m0);
	ret = reach.insert (m);
	work.push (m);

	while (! work.empty ()) {
		m = work.front ();
		work.pop ();
		m->enabled (ena);

		for (auto t = ena.begin (); t != ena.end (); t++) {
			Marking * mp = new Marking (*m);
			mp->fire (**t);
			ret = reach.insert (mp);
			/* std::cout << *m << " firing " << **t << " yields ";
			std::cout << *mp << " new? " << ret.second <<
				std::endl; */
			if (ret.second) {
				work.push (mp);
				if (*mp == target) {
					result = true;
					goto __cleanup;
				}
			} else {
				delete mp;
			}
		}
	}
	result = false;

__cleanup :
	for (auto it = reach.begin (); it != reach.end (); it++) delete *it;
	return result;
}


Marking::Marking (const Net & n) :
	mrk (n.places.size ()),
	net (&n)
{
	TRACE (&n, "p");
	TRACE (net, "p");
}

unsigned long Marking::operator[] (const Place & p) const
{
	ASSERT (p.id < mrk.size ());
	return mrk[p.id];
}

unsigned long & Marking::operator[] (const Place & p)
{
	ASSERT (p.id < mrk.size ());
	return mrk[p.id];
}

bool Marking::__find_diff (const Marking & m,
		std::vector<unsigned long>::size_type & i) const
{
	std::vector<unsigned long>::size_type j;

	for (j = 0; j < mrk.size (); j++) {
		if (mrk[j] != m.mrk[j]) {
			i = j;
			return true;
		}
	}
	return false;
}

bool Marking::operator== (const Marking & m) const
{
	std::vector<unsigned long>::size_type i;
	if (net != m.net) return false;
	ASSERT (mrk.size () == m.mrk.size ());
	// std::cout << "==" << *this << " " << m << std::endl;
	return ! __find_diff (m, i);
}

bool Marking::operator< (const Marking & m) const
{
	std::vector<unsigned long>::size_type i;
	if (net != m.net) return false;
	ASSERT (mrk.size () == m.mrk.size ());
	if (__find_diff (m, i)) return mrk[i] < m.mrk[i];
	return false;
}

bool Marking::operator> (const Marking & m) const
{
	std::vector<unsigned long>::size_type i;
	if (net != m.net) return false;
	ASSERT (mrk.size () == m.mrk.size ());
	if (__find_diff (m, i)) return mrk[i] > m.mrk[i];
	return false;
}

bool Marking::is_enabled (const Trans & t) const
{
	for (auto it = t.pre.begin (); it != t.pre.end (); it++) {
		ASSERT ((*it)->id < mrk.size ());
		if (mrk[(*it)->id] == 0) return false;
	}
	for (auto it = t.cont.begin (); it != t.cont.end (); it++) {
		ASSERT ((*it)->id < mrk.size ());
		if (mrk[(*it)->id] == 0) return false;
	}
	return true;
}

void Marking::enabled (std::vector<Trans *> & list) const
{
	list.clear ();
	for (auto t = net->trans.begin (); t != net->trans.end (); t++) {
		auto p = (*t)->pre.begin ();
		for (; p != (*t)->pre.end (); p++) {
			if (mrk[(*p)->id] == 0) break;
		}
		if (p != (*t)->pre.end ()) continue;

		for (p = (*t)->cont.begin(); p != (*t)->cont.end (); p++) {
			if (mrk[(*p)->id] == 0) break;
		}
		if (p == (*t)->cont.end ()) list.push_back ((Trans *) *t);
	}
}

void Marking::fire (const Trans & t)
{
	ASSERT (is_enabled (t));
	for (auto p = t.pre.begin (); p != t.pre.end (); p++) {
		mrk[(*p)->id]--;
	}

	for (auto p = t.post.begin (); p != t.post.end (); p++) {
		ASSERT ((*p)->id < mrk.size ());
		mrk[(*p)->id]++;
	}
}

void Marking::resize (void)
{
	mrk.resize (net->places.size ());
}

void Marking::clear (void) {
	for (auto p = mrk.begin (); p != mrk.end (); p++) *p = 0;
}


void net_net_test1 (void)
{
	Net n;

	Place & p1 = n.place_add ("p1", 2);
	Place & p2 = n.place_add ("p2", 4);
	Place & p3 = n.place_add ("p3", 1);
	Place & p4 = n.place_add ("p4");

	Trans & t1 = n.trans_add ("t1");
	Trans & t2 = n.trans_add ("t2");

	t1.pre_add  (p1);
	t1.cont_add (p2);
	t1.post_add (p3);
	
	t2.pre_add  (p2);
	t2.post_add (p4);

	Marking m = n.m0;
#if 0
	std::vector<Trans *> ena;
	while (true) {
		m.enabled (ena);
		std::cout << m << " enables " << ena << std::endl;
		if (ena.size () == 0) break;
		m.fire (*ena[0]);
	}
	return;
#endif

	m.clear ();
	m[p1] = 2;
	m[p3] = 1;
	m[p4] = 4;
	std::cout << m << " is reachable? " << std::endl;
	std::cout << n.is_reachable (m) << std::endl;
}

