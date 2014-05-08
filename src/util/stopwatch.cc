
#include "util/misc.h"
#include "util/stopwatch.hh"


Stopwatch::Stopwatch ()
	: running(true),  started(clock ()), count(0), s()
{}

void Stopwatch::start ()
{
	running = true;
	count = 0;
	started = clock ();
}

void Stopwatch::stop ()
{
	clock_t now;

	now = clock ();
	if (! running) return;
	running = false;
	count += now - started;
}

void Stopwatch::resume ()
{
	if (running) return;
	running = true;
	started = clock ();
}

const std::string & Stopwatch::print ()
{
	clock_t now = clock ();
	clock_t ticks;

	ticks = count;
	if (running) ticks += now - started;
	
	s = fmt ("%ldms", ticks / (CLOCKS_PER_SEC / 1000));
	return s;
}

const char * Stopwatch::c_str ()
{
	print ();
	return s.c_str ();
}

