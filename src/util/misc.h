
#ifndef _UTIL_MISC_H_
#define _UTIL_MISC_H_

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "util/config.h"

#ifdef __cplusplus
extern "C" {
#endif

// the different levels of verbosity in the program
#define VERB_DEBUG	3
#define VERB_TRACE	2
#define VERB_INFO		1
#define VERB_PRINT	0

// the "verbosity" of the program; initially set to VERB_DEBUG
extern int __verb_level;

// these evaluate to true or false depending on the current verbosity level
extern int verb_debug;
extern int verb_trace;
extern int verb_info;

// setting and getting the verbosity level
void verb_set (int i);
int verb_get ();

// the actual primitives you should use, with and without new line
#define DEBUG(fmt,args...)		mylog (3, fmt "\n", ##args)
#define TRACE(fmt,args...)		mylog (2, fmt "\n", ##args)
#define INFO(fmt,args...)		mylog (1, fmt "\n", ##args)
#define PRINT(fmt,args...)		mylog (0, fmt "\n", ##args)

#define DEBUG_(fmt,args...)	mylog (3, fmt, ##args)
#define TRACE_(fmt,args...)	mylog (2, fmt, ##args)
#define INFO_(fmt,args...)		mylog (1, fmt, ##args)
#define PRINT_(fmt,args...)	mylog (0, fmt, ##args)

// the implementation
inline void mylog (int level, const char * fmt, ...)
{
	va_list ap;

	if (level > CONFIG_MAX_VERB_LEVEL) return;
	if (level > __verb_level) return;
	va_start (ap, fmt);
	vprintf (fmt, ap);
	va_end (ap);
}

#if CONFIG_MAX_VERB_LEVEL >= 3
#define VERB_LEVEL_DEBUG
#endif
#if CONFIG_MAX_VERB_LEVEL >= 2
#define VERB_LEVEL_TRACE
#endif
#if CONFIG_MAX_VERB_LEVEL >= 1
#define VERB_LEVEL_INFO
#endif


// more debugging primitives
void breakme (void);
#define BREAK(expr)			if (expr) breakme ()
#define SHOW(expr,type)	DEBUG (#expr "='%" type "'", expr)
#ifdef CONFIG_DEBUG
#define ASSERT(expr)    \
	if (! (expr)) { \
		PRINT (__FILE__ ":%d: %s: Assertion `" #expr "' failed.\n", \
				__LINE__, __func__); \
		breakme (); \
		exit (1); \
	}
#endif


// additional stuff for C++
#ifdef __cplusplus
} // extern "C"

#include <string>
std::string fmt (const std::string fmt_str, ...);

// FIXME uncomment this
/*inline void mylog (int level, const std::string & s)
	{ mylog (level, "%s", s.c_str ()); } */

#endif // __cplusplus

#endif // _UTIL_MISC_H_
