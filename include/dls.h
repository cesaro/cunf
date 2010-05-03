
/*
 * Static doubly-linked list
 */

#ifndef _DLS_H_
#define _DLS_H_

struct dls {
	struct dls * prev;
	struct dls * next;
};

void dls_init (struct dls * l);
void dls_insert (struct dls * l, struct dls * n);
void dls_append (struct dls * l, struct dls * n);
void dls_remove (struct dls * l, struct dls * n);
void dls_shift (struct dls * l);
int dls_iter (struct dls * l, int (* callback) (struct dls * n));
void dls_print (struct dls * l, const char * heading);

void dls_push (struct dls * l, struct dls * n);
struct dls * dls_pop (struct dls * l);

#define dls_item(type,nod,field) \
		((type *) ((void *) (nod) - (void *) &((type *) 0)->field))

#define dls_i dls_item

#endif

