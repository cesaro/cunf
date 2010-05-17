
#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "ls.h"
#include "dg.h"
#include "nodelist.h"

/****************************************************************************/
/* FIXME structures for places, transitions, conditions, events		    */
/* FIXME -- assert that presets are always at least of size 1 */

/* If you change these structures, do keep the "next" field at the top;
   reverse_list() depends on it.					    */

struct place {
	struct ls nod;
	char  *name;	    /* short name			    */
	int    id;	    /* number				    */
	struct dg pre;     /* unordered list of preset		    */
	struct dg post;    /* unordered list of postset	    */
	struct dg cont;    /* read arcs (unordered)    	    */
	struct nl *conds;   /* conditions derived from this place   */
	int m;	    	/* non-zero if place is marked	    */
};

struct trans {
	struct ls nod;
	char  *name;	    /* short name			    */
	int    id;	    /* number				    */
	struct dg pre;     /* unordered list of preset		    */
	struct dg post;    /* unordered list of postset	    */
	struct dg cont;    /* read arcs (unordered)    	    */
	int m;
};

struct cond {
	struct ls nod;			/* node for the list of conditions */
	struct event *pre;		/* preset, only one event */
	struct dg post;			/* postset, list of events */
	struct dg cont;			/* context, list of events */
	struct place *origin;		/* associated place */
	int    id;			/* number (needed by co_relation) */
	int    m;			/* mark, used by multiple functions */
	int    causalm;			/* see function _pe_update_post_existing */
};

struct event {
	struct ls nod;			/* node for the list of events */
	struct dg pre;			/* preset, list of conditions */
	struct dg post;			/* postset, list of conditions */
	struct dg cont;			/* context, list of conditions */
	struct dg ac;			/* asymmetric conflict rel. node */
	struct dg hist;			/* histories associated to the event */
	struct trans *origin;		/* associated transition */
	int    id;
	int iscutoff;
	int m;
};

struct net {
	struct ls places;	/* pointer to first place		*/
	struct ls trans;	/* pointer to first transition		*/ 
	int numpl, numtr;	/* number of places/transitions in net	*/
	int maxpre, maxpost;	/* maximal size of a t-pre/postset	*/
	int maxcont;
	struct trans *t0;
};

struct unf {
	struct ls conds;	/* list of conditions */
	struct ls events;	/* list of events */
	int numco, numev;	/* number of conditions/events in net	*/
	int numh;		/* number if histories */
	struct event *e0;	/* event generating the minimal conditions */
};

struct u {
	struct net net;
	struct unf unf;

	int mark;

	struct trans * stoptr;
	int depth;
	int interactive;
};

struct u u;

#endif

