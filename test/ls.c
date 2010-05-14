
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>

#include "glue.h"
#include "debug.h"
#include "ls.h"
#include "dg.h"

struct mynode {
	int data;
	struct ls nod;
};

#define MAX 100000

static int _print (struct ls * n)
{
	struct mynode * mn;

	mn = ls_item (struct mynode, n, nod);
	printf ("ptr %p data %4d next %p\n", n, mn->data, mn->nod.next);
	return 0;
}

void main1 (void)
{
	int i;
	struct ls l;
	struct mynode nods[MAX];
	struct ls * n;
	struct mynode * mn;

	ls_init (&l);

	for (i = 0; i < MAX; i++) {
		nods[i].data = i;
		if (i % 3 == 0) ls_insert (&l, &nods[i].nod);
	}
		
	ls_remove (&l, &nods[3].nod);
	printf ("After removing next to nods[3]:\n");
	ls_iter (&l, _print);

	ls_remove (&l, 0);
	printf ("After removing the first:\n");
	ls_iter (&l, _print);

	/* empty the list */
	while (ls_pop (&l));
	printf ("After emptying:\n");
	ls_iter (&l, _print);

	/* push some elements */
	ls_push (&l, &nods[1].nod);
	ls_push (&l, &nods[10].nod);
	ls_push (&l, &nods[15].nod);
	printf ("After pushing:\n");
	ls_iter (&l, _print);

	n = ls_pop (&l);
	ASSERT (n);
	mn = ls_item (struct mynode, n, nod);
	printf ("Pop: data %d:\n", mn->data);

	n = ls_pop (&l);
	ASSERT (n);
	mn = ls_item (struct mynode, n, nod);
	printf ("Pop: data %d:\n", mn->data);

	n = ls_pop (&l);
	ASSERT (n);
	mn = ls_item (struct mynode, n, nod);
	printf ("Pop: data %d:\n", mn->data);

	n = ls_pop (&l);
	ASSERT (n == 0);
}

int main (void)
{
	main1 ();
	return 0;
}

