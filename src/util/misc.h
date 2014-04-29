
#ifndef _UTIL_MISC_H_
#define _UTIL_MISC_H_

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "util/config.h"

#ifdef __cplusplus
extern "C" {
#endif

// the "verbosity" of the program; by default it is 5, everything is logged
extern int __log_level;

// setting and getting the log level
void log_set_level (int i);
int log_get_level ();

// the actual primitives you should use, with and without new line
#define DEBUG(fmt,args...)		mylog (5, fmt "\n", ##args)
#define TRACE(fmt,args...)		mylog (4, fmt "\n", ##args)
#define INFO(fmt,args...)		mylog (3, fmt "\n", ##args)
#define WARN(fmt,args...)		mylog (2, fmt "\n", ##args)
#define ERROR(fmt,args...)		mylog (1, fmt "\n", ##args)
#define FATAL(fmt,args...)		mylog (0, fmt "\n", ##args)

#define DEBUG_(fmt,args...)	mylog (5, fmt, ##args)
#define TRACE_(fmt,args...)	mylog (4, fmt, ##args)
#define INFO_(fmt,args...)		mylog (3, fmt, ##args)
#define WARN_(fmt,args...)		mylog (2, fmt, ##args)
#define ERROR_(fmt,args...)	mylog (1, fmt, ##args)
#define FATAL_(fmt,args...)	mylog (0, fmt, ##args)

// the implementation
inline void mylog (int level, const char * fmt, ...)
{
	va_list ap;

	if (level > CONFIG_MAX_LOG_LEVEL) return;
	if (level > __log_level) return;
	va_start (ap, fmt);
	vprintf (fmt, ap);
	va_end (ap);
}


// more debugging primitives
void breakme (void);
#define BREAK(expr)			if (expr) breakme ()
#define PRINT(expr,type)	DEBUG (#expr "='%" type "'", expr)
#ifdef CONFIG_DEBUG
#define ASSERT(expr)    \
	if (! (expr)) { \
		FATAL (__FILE__ ":%d: %s: Assertion `" #expr "' failed.\n", \
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
