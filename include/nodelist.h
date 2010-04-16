
#ifndef _NODELIST_H_
#define _NODELIST_H_

struct nl {
	void * node;
	struct nl * next;
};

struct nl * nl_alloc ();
struct nl * nl_push (struct nl **, void *);
struct nl * nl_insert (struct nl **, void*);
void nl_delete (struct nl *);
char nl_compare (struct nl *, struct nl *);

#endif
