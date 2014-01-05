
#ifndef _NET_NET_HH_
#define _NET_NET_HH_

#include <vector>
#include <string>
#include <ostream>

class Place;

class Trans
{
public:
	std::vector<Place *>::size_type id;
	std::string name;
	std::vector<Place *> pre;
	std::vector<Place *> post;
	std::vector<Place *> cont;
	int m;

	Trans (const std::string & name,
			std::vector<Place *>::size_type idx=0);
	~Trans ();

	void pre_add (Place & p, bool call_oppst=true);
	void post_add (Place & p, bool call_oppst=true);
	void cont_add (Place & p, bool call_oppst=true);

	void pre_rem (Place & p, bool call_oppst=true);
	void post_rem (Place & p, bool call_oppst=true);
	void cont_rem (Place & p, bool call_oppst=true);
};

class Place
{
public:
	std::vector<Place *>::size_type id;
	std::string name;
	std::vector<Trans *> pre;
	std::vector<Trans *> post;
	std::vector<Trans *> cont;
	unsigned long m;

	Place (const std::string & name,
			std::vector<Place *>::size_type idx=0);
	~Place ();

	void pre_add (Trans & t, bool call_oppst=true);
	void post_add (Trans & t, bool call_oppst=true);
	void cont_add (Trans & t, bool call_oppst=true);

	void pre_rem (Trans & t, bool call_oppst=true);
	void post_rem (Trans & t, bool call_oppst=true);
	void cont_rem (Trans & t, bool call_oppst=true);
};

class Net;

class Marking
{
	std::vector<unsigned long> mrk;
	const Net * net;
	bool __find_diff (const Marking & m,
			std::vector<unsigned long>::size_type & i) const;
public:
	Marking (const Net & net);
	bool is_enabled (const Trans & t) const;
	void enabled (std::vector<Trans *> & list) const;
	void fire (const Trans & t);
	void resize (void);
	void clear (void);

	unsigned long   operator[] (const Place & p) const;
	unsigned long & operator[] (const Place & p);
	bool operator== (const Marking & m) const;
	bool operator< (const Marking & m) const;
	bool operator> (const Marking & m) const;

	friend std::ostream & operator<< (std::ostream & os, const Marking & m);
};

class Net
{
public:
	std::vector<Place *> places;
	std::vector<Trans *> trans;
	Marking m0;

	std::string author;
	std::string title;
	std::string date;
	std::string note;
	std::string version;


	Net (void);
	Place & place_add (const std::string name, unsigned long m=0);
	Trans & trans_add (const std::string name);

	bool is_reachable (const Marking & target) const;
};

std::ostream & operator<< (std::ostream & os, const Trans & t);
std::ostream & operator<< (std::ostream & os, const Place & p);
std::ostream & operator<< (std::ostream & os, const Marking & m);

template<typename T>
std::ostream & operator<< (std::ostream & os, const std::vector<T> & v);
template<typename T>
std::ostream & operator<< (std::ostream & os, const std::vector<T *> & v);

void net_net_test1 (void);


#endif

