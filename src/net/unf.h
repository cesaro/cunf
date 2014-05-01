
#ifndef _NET_UNF_HH_
#define _NET_UNF_HH_

#include <vector>
#include <string>
#include <ostream>

#include "net/net.hh"

class Cond;

class Event :
{
public:
	std::vector<Event *>::size_type id;
	std::string name;
	std::vector<Cond *> pre;
	std::vector<Cond *> post;
	std::vector<Cond *> cont;
	int m;

	Event (const Trans * t,
			std::vector<Event *>::size_type idx=0);
	~Event ();

	void pre_add (Cond & c, bool call_oppst=true);
	void post_add (Cond & c, bool call_oppst=true);
	void cont_add (Cond & c, bool call_oppst=true);

	void pre_rem (Cond & c, bool call_oppst=true);
	void post_rem (Cond & c, bool call_oppst=true);
	void cont_rem (Cond & c, bool call_oppst=true);
};

class Cond
{
public:
	std::vector<Cond *>::size_type id;
	std::string name;
	Event * pre;
	std::vector<Event *> post;
	std::vector<Event *> cont;
	unsigned long m;

	Cond (const Place * p, Event * e,
			std::vector<Cond *>::size_type idx=0);
	~Cond ();

	void pre_add (Event & e, bool call_oppst=true);
	void post_add (Event & e, bool call_oppst=true);
	void cont_add (Event & e, bool call_oppst=true);

	void pre_rem (Event & e, bool call_oppst=true);
	void post_rem (Event & e, bool call_oppst=true);
	void cont_rem (Event & e, bool call_oppst=true);
};

class Net;

class Marking
{
	std::vector<bool> mrk;
	const Unf * unf;
	bool __find_diff (const Marking & m,
			std::vector<unsigned long>::size_type & i) const;
public:
	Marking (const Net & net);
	bool is_enabled (const Event & t) const;
	void enabled (std::vector<Event *> & list) const;
	void fire (const Event & t);
	void resize (void);
	void clear (void);

	unsigned long   operator[] (const Cond & p) const;
	unsigned long & operator[] (const Cond & p);
	bool operator== (const Marking & m) const;
	bool operator< (const Marking & m) const;
	bool operator> (const Marking & m) const;

	friend std::ostream & operator<< (std::ostream & os, const Marking & m);
};

class Unf
{
public:
	std::vector<Cond *> places;
	std::vector<Event *> trans;
	Marking m0;

	std::string author;
	std::string title;
	std::string date;
	std::string note;
	std::string version;


	Net (void);
	Cond & place_add (const std::string name, unsigned long m=0);
	Event & trans_add (const std::string name);

	bool is_reachable (const Marking & target) const;
};

std::ostream & operator<< (std::ostream & os, const Event & t);
std::ostream & operator<< (std::ostream & os, const Cond & p);
std::ostream & operator<< (std::ostream & os, const Marking & m);

template<typename T>
std::ostream & operator<< (std::ostream & os, const std::vector<T> & v);
template<typename T>
std::ostream & operator<< (std::ostream & os, const std::vector<T *> & v);

void net_net_test1 (void);


#endif

