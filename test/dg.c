
#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#include "glue.h"
#include "debug.h"
#include "ls.h"
#include "dg.h"

struct mynode {
	int data;
	struct dg nod;
};

struct mynode2 {
	int data;
	struct ls nod;
};

#define MAX 100000
#define REP 100000

void main1 (void)
{
	struct mynode nods[MAX];
	struct mynode * mn;
	register int i;
	int j, sum;

	for (i = 0; i < MAX; i++) {
		nods[i].data = i;
		dg_init (&nods[i].nod);
	}

	dg_add (&nods[0].nod, &nods[1].nod);
	dg_add (&nods[0].nod, &nods[2].nod);
	dg_add (&nods[0].nod, &nods[7].nod);
	dg_add (&nods[0].nod, &nods[8].nod);

	for (i = 0; i < MAX; i++) {
		dg_add (&nods[5].nod, &nods[i].nod);
	}

	printf (" adj (%d, %d) is %d\n", 0, 1, dg_test (&nods[0].nod, &nods[1].nod));
	printf (" adj (%d, %d) is %d\n", 0, 10, dg_test (&nods[0].nod, &nods[10].nod));

	for (i = 0; i < nods[0].nod.deg; i++) {
		mn = dg_item (struct mynode, nods[0].nod.adj[i], nod);
		printf (" edge (0, %d)\n", mn->data);
	}

	sum = 0;
	for (j = 0; j < REP; j++) {
		for (i = 0; i < nods[5].nod.deg; i++) {
			mn = dg_item (struct mynode, nods[5].nod.adj[i], nod);
			// printf (" edge (5, %d)\n", mn->data);
			sum += mn->data;
			TRACE (mn->data, "d");
		}
	}
	printf ("sum = %d!\n", sum);
}

static int _print (struct ls * n)
{
	struct mynode2 * mn;

	mn = ls_item (struct mynode2, n, nod);
	printf ("ptr %p data %4d next %p\n", n, mn->data, mn->nod.next);
	return 0;
}

void main2 (void)
{
	int i;
	struct ls l;
	struct mynode2 nods[MAX];
	struct ls * n;
	struct mynode2 * mn;

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
	mn = ls_item (struct mynode2, n, nod);
	printf ("Pop: data %d:\n", mn->data);

	n = ls_pop (&l);
	ASSERT (n);
	mn = ls_item (struct mynode2, n, nod);
	printf ("Pop: data %d:\n", mn->data);

	n = ls_pop (&l);
	ASSERT (n);
	mn = ls_item (struct mynode2, n, nod);
	printf ("Pop: data %d:\n", mn->data);

	n = ls_pop (&l);
	ASSERT (n == 0);
}

int main (void)
{
	main1 ();
	// main2 ();
	return 0;
}

