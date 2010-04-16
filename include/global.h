
#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "ls.h"
#include "dg.h"
#include "nodelist.h"

/****************************************************************************/
/* FIXME structures for places, transitions, conditions, events		    */

/* If you change these structures, do keep the "next" field at the top;
   reverse_list() depends on it.					    */

struct place {
	struct ls nod;
	char  *name;	    /* short name			    */
	int    num;	    /* number				    */
	struct nl *pre;     /* unordered list of preset		    */
	struct nl *post;    /* unordered list of postset	    */
	struct nl *cont;    /* read arcs (unordered)    	    */
	struct nl *conds;   /* conditions derived from this place   */
	char marked;	    /* non-zero if place is marked	    */
};

struct trans {
	struct ls nod;
	char  *name;	    /* short name			    */
	int    num;	    /* number				    */
	struct nl *pre;     /* unordered list of preset		    */
	struct nl *post;    /* unordered list of postset	    */
	struct nl *cont;    /* read arcs (unordered)    	    */
	short  pre_size, post_size, cont_size;
};

struct cond {
	struct ls nod;			/* list of conditions */
	struct event_t *pre;		/* the single event in the preset */
	struct dg post;			/* unordered list of postset */
	struct place *origin;		/* associated place */
	int    num;			/* number (needed by co_relation) */
	int    m;			/* mark, used by multiple functions */
};

struct event {
	struct ls nod;			/* list of events */
	struct dg pre;			/* preset, list of conditions */
	struct dg post;			/* postset, list of conditions */
	struct trans *origin;		/* associated transition */
	int    m;			/* mark, used by multiple functions */
	int    num;
	short  foata_level;
};

struct net {
	struct ls places;	/* pointer to first place		*/
	struct ls trans;	/* pointer to first transition		*/ 
	int numpl, numtr;	/* number of places/transitions in net	*/
	int maxpre, maxpost;	/* maximal size of a t-pre/postset	*/
	int maxcont;
};

struct unf {
	struct ls conds;	/* list of conditions */
	struct ls events;	/* list of events */
	int numco, numev;	/* number of conditions/events in net	*/
	struct event *e0;	/* event generating the minimal conditions */
};

struct u {
	struct net net;
	struct unf unf;
	int mark;

	char *stoptr;
	int depth;
	int interactive;
	int exitcode;
};

struct u u;

#endif

