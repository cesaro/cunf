
/*
 * Static Singly-Linked List with Tail -- interface
 */

#ifndef _LST_LST_H_
#define _LST_LST_H_

struct lst {
	struct lstn * head;
	struct lstn * tail;
};

struct lstn {
	struct lstn * next;
};

void lst_init (struct lst * l);
void lst_insert (struct lst * l, struct lstn * n);
void lst_append (struct lst * l, struct lstn * n);
void lst_remove (struct lst * l, struct lstn * p);
void lst_shift (struct lst * l);
int lst_iter (struct lst * l, int (* callback) (struct lstn * n));
void lst_print (struct lst * l, const char * heading);

void lst_push (struct lst * l, struct lstn *n);
struct lstn * lst_pop (struct lst * l);

#define lst_item(type,nod,field) \
		((type *) ((void *) (nod) - (void *) &((type *) 0)->field))

#define lst_i lst_item

#endif

