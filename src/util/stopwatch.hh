
#ifndef _UTIL_STOPWATCH_HH_
#define _UTIL_STOPWATCH_HH_

#include <ctime>

class Stopwatch
{
public :
	Stopwatch ();

	void start ();
	void stop ();
	void resume ();
	const std::string & print ();
	const char * c_str ();

private :
	bool running;
	clock_t started;
	clock_t count;
	std::string s;
};

#endif // _UTIL_STOPWATCH_HH_
