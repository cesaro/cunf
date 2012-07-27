
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

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "ls/ls.h"
#include "al/al.h"

struct place {
	struct ls nod;	/* node for the list of places */
	char *name;	/* place name, from the ll_net format */
	int id;		/* internal numeric identifier */
	struct al pre;	/* adjacency list, preset transitions */
	struct al post;	/* postset transitions */
	struct al cont;	/* transitions in the context */
	struct ls conds; /* conditions derived from this place   */
	int m;		/* general purpose mark; initially, non-zero if place
			 * is marked */
	int comb_idx;	/* see _pe_comb_init() */
};

struct trans {
	struct ls nod;	/* node for the list of transitions */
	char *name;	/* transition name, from the ll_net format */
	int id;		/* internal numeric identifier */
	struct al pre;	/* unordered list of preset		    */
	struct al post;	/* unordered list of postset	    */
	struct al cont;	/* read arcs (unordered)    	    */
	struct ls events; /* events derived from this place */
	int m;		/* general purpose mark */
	int parikhcnt1;	/* see function FIXME */
	int parikhcnt2;	/* see function FIXME */
};

struct cond {
	struct ls nod;		/* node for the list of conditions */
	struct event *pre;	/* preset, only one event */
	struct al post;		/* postset, list of events */
	struct al cont;		/* context, list of events */
	struct place *fp;	/* associated place */
	struct ls pnod;		/* conditions assoc. to same place */
	struct ls ecl;		/* list of associated enriched conditions */
	int id;			/* internal numeric identifier */
	int m;			/* general purpose mark */
	int cnt;		/* see ec_conc */
};

struct event {
	struct ls nod;		/* node for the list of events */
	struct al pre;		/* preset, list of conditions */
	struct al post;		/* postset, list of conditions */
	struct al cont;		/* context, list of conditions */
	struct al ac;		/* asymmetric conflict rel. node */
	struct al hist;		/* histories associated to the event */
	struct trans *ft;	/* associated transition */
	struct ls tnod;		/* events associated to same place */
	int id;			/* event identifier */
	int iscutoff;		/* true if all histories are cutoffs */
	int m;			/* general purpose mark */
};

struct net {
	struct ls places;	/* list of places */
	struct ls trans;	/* list of transitions */
	int numpl, numtr;	/* number of places/transitions in net*/
	struct trans *t0;	/* fake trans. generating initial marking */
	int isplain;		/* true iff there is no read arcs */
};

struct unf {
	struct ls conds;	/* list of conditions */
	struct ls events;	/* list of events */
	int numcond, numev;	/* number of conditions/events in net	*/
	int numh;		/* number of histories */
	int numcutoffs;		/* number of cutoff histories */
	int numgen;		/* number of generating ecs */
	int numread;		/* number of reading ecs */
	int numcomp;		/* number of compound ecs */

	int numr;		/* number of pairs in the R relation */
	int nums;		/* number of pairs in the S relation */
	unsigned long long numco; /* number of pairs in the concurrency rel. */
	unsigned long long numrco; /* number of pairs in r->rco arrays */
	unsigned long long nummrk; /* number of conditions in the marking lists */
	unsigned long long numewhite; /* number of events without cutoff histories */
	unsigned long long numegray; /* ... with at least 1 cutoff and 1 non cutoff */
	unsigned long long numeblack; /* ... with only cutoff histories */

	int numepost;		/* number of conditions event postsets */
	int numecont;		/* number of conditions event contexts */
	int numepre;		/* number of conditions event presets */

	long usrtime;		/* user time of the process (in ms) */
	long vmsize;		/* current virtual memory size (in kb) */

	struct event *e0;	/* event generating the minimal conditions */
};

struct u {
	struct net net;
	struct unf unf;

	int mark;

	struct trans * stoptr;
	int depth;
	/* int interactive; */
};

struct u u;

#endif

