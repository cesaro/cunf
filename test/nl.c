

#include "debug.h"
#include "glue.h"
#include "nodelist.h"

#define MAX 10

int main (void)
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

	return 0;
}

