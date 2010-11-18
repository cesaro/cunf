
#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "ec.h"
#include "h.h"

#define PRINT(args...) fprintf (stderr, ##args)
/* #define PRINT printf */

#ifdef CONFIG_DEBUG
#define ASSERT(expr)    \
	if (! (expr)) { \
		PRINT (__FILE__ ":%d: %s: Assertion `" #expr "' failed.\n", \
				__LINE__, __func__); \
		breakme (); \
		exit (1); \
	}

#define DPRINT			PRINT
#define DEBUG(fmt,args...)      PRINT ("%s: " fmt "\n", __func__, ##args)
#define TRACE(expr,type)        DEBUG (#expr "='%" type "'", expr)
#define BREAK(expr)    		if (expr) breakme ()

#else
#undef _DEBUG
#define DPRINT(args...)
#define ASSERT(expr)
#define DEBUG(fmt,args...)
#define TRACE(expr,type)
#define BREAK(expr)
#endif

void breakme (void);

void db_net (void);
void db_e (struct event *e);
void db_c (struct cond *c);
void db_h (struct h *h);
void db_r (struct ec *r);
void db_r2 (const char *str1, struct ec *r, const char *str2);
void db_hgraph (void);
void db_h2dot (void);

#endif

