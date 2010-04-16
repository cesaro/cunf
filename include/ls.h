
/*
 * Static single-linked list -- interface
 */

#ifndef _LS_LS_H_
#define _LS_LS_H_

struct ls {
	struct ls * next;
};

void ls_init (struct ls * l);
void ls_insert (struct ls * l, struct ls * n);
void ls_append (struct ls * l, struct ls * n);
void ls_remove (struct ls * l, struct ls * p);
void ls_shift (struct ls * l);
int ls_iter (struct ls * l, int (* callback) (struct ls * n));
void ls_print (struct ls * l, const char * heading);

void ls_push (struct ls * l, struct ls *n);
struct ls * ls_pop (struct ls * l);

#define ls_isempty(l) ((l)->next == 0)
/*
#define ls_item(type,nod,field) \
		ASSERT (nod); \
		ASSERT (sizeof (type) > sizeof (struct er_nod)); \
		((type *) ((void *) (nod) - (void *) &((type *) 0)->field))
*/
#define ls_item(type,nod,field) \
		((type *) ((void *) (nod) - (void *) &((type *) 0)->field))


#endif

