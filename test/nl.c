
#include "debug.h"
#include "glue.h"
#include "nodelist.h"

#define MAX 10

void main1 (void)
{
	int i;
	struct nl * l;
	struct nl * n;
	struct data {
		int nr;
	} data[MAX];

	for (i = 0; i < MAX; i++) data[i].nr = i;

	l = 0;
	for (i = 0; i < MAX; i++) {
		nl_push (&l, data + i); 
	}

	for (n = l->next; n; n = n->next) {
		printf ("i %4d  n->node->nr %4d\n",
				i,
				((struct data *) n->node)->nr);
	}
}

static int _cmp (void * p1, void *p2)
{
	// return (* (int *) p1) - (* (int *) p2);
	int a, b, ret;
	a = * (int *) p1;
	b = * (int *) p2;
	ret = a - b;
	return ret;
}

void main2 (void)
{
	struct nl *l, *ll, *n, *p;
	int i, d, found;
	int data[MAX];

	l = 0;
	ll = 0;
	d = MAX / 2;
	for (i = 0; i < MAX; i++) {
		data[i] = d % MAX;
		d++;
		nl_insert (&l, data + i);
		nl_insert2 (&ll, data + i, _cmp);
	}

	/* assert that there is a non-ordered pair in list l*/
	found = 0;
	i = 0;
	for (n = l, p = 0; n; p = n, n = n->next) {
		// DEBUG ("i %d n %p n"
		if (p) found = (* (int *) p->node) > (* (int *) n->node);
		if (found) break;
	}
	ASSERT (found);

	/* assert that there is not a non-ordered pair in list ll */
	found = 0;
	for (n = ll, p = 0; n; p = n, n = n->next) {
		if (p) found = (* (int *) p->node) > (* (int *) n->node);
		if (found) break;
	}
	ASSERT (found == 0);
}

int main (void)
{
	main1 ();
	main2 ();
	return 0;
}

