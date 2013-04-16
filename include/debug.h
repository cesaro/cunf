
/* 
 * Copyright (C) 2010, 2011  Cesar Rodriguez <cesar.rodriguez@lsv.ens-cachan.fr>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>
#include <stdlib.h>

#define PRINT(args...) fprintf (stdout, ##args)
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
#define BREAK(expr)    		if (expr) breakme ()
//#define BREAK(expr)
#endif

struct event;
struct cond;
struct h;
struct ec;

void breakme (void);

void db_net (void);
void db_e (struct event *e);
void db_c (struct cond *c);
void db_h (struct h *h);
void db_r (struct ec *r);
void db_r2 (const char *str1, struct ec *r, const char *str2);
void db_hgraph (void);
void db_h2dot (void);
void db_mem (void);

#endif

