
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
	struct dg nod;
};

#define MAX 100000
#define REP 100000

void db_adj2 (struct dg * n, struct mynode *p)
{
	int i;
	struct mynode * mn;

	mn = dg_i (struct mynode, n, nod);
	PRINT (" ** nr %d node %p deg %d\n", mn - p, n, n->deg);
	if (n->deg == 0) return;

	for (i = 0; i <= n->deg; i++) {
		PRINT ("i %4d ptr %p", i, n->adj[i]);
		if (n->adj[i]) {
			mn = dg_i (struct mynode, n->adj[i], nod);
			PRINT (" (%d)", mn - p);
		}
		PRINT ("\n");
	}
}

void main1 (void)
{
	struct mynode nods[MAX];
	struct mynode * mn;
	register int i;
	int j, sum;

	ASSERT (MAX >= 10);
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

	PRINT (" adj (%d, %d) is %d\n", 0, 1, dg_test (&nods[0].nod, &nods[1].nod));
	PRINT (" adj (%d, %d) is %d\n", 0, 10, dg_test (&nods[0].nod, &nods[10].nod));

	for (i = 0; i < nods[0].nod.deg; i++) {
		mn = dg_item (struct mynode, nods[0].nod.adj[i], nod);
		PRINT (" edge (0, %d)\n", mn->data);
	}

	sum = 0;
	for (j = 0; j < REP; j++) {
		for (i = 0; i < nods[5].nod.deg; i++) {
			mn = dg_item (struct mynode, nods[5].nod.adj[i], nod);
			// PRINT (" edge (5, %d)\n", mn->data);
			sum += mn->data;
			TRACE (mn->data, "d");
		}
	}
	PRINT ("sum = %d!\n", sum);
}

extern uint32_t _dg_nextpow (uint32_t i);

void main3 (void)
{
	/* test function _dg_nextpow :) */

	ASSERT (_dg_nextpow (0) == 2);
	ASSERT (_dg_nextpow (1) == 0x2);
	ASSERT (_dg_nextpow (2) == 0x2);
	ASSERT (_dg_nextpow (3) == 0x4);
	ASSERT (_dg_nextpow (4) == 0x4);
	ASSERT (_dg_nextpow (5) == 0x8);
	ASSERT (_dg_nextpow (6) == 0x8);
	ASSERT (_dg_nextpow (7) == 0x8);
	ASSERT (_dg_nextpow (8) == 0x8);
	ASSERT (_dg_nextpow (9) == 0x10);
	ASSERT (_dg_nextpow (10) == 0x10);
	ASSERT (_dg_nextpow (15) == 0x10);
	ASSERT (_dg_nextpow (16) == 0x10);
	ASSERT (_dg_nextpow (17) == 0x20);
	ASSERT (_dg_nextpow (18) == 0x20);
	ASSERT (_dg_nextpow (31) == 0x20);
	ASSERT (_dg_nextpow (32) == 0x20);
	ASSERT (_dg_nextpow (33) == 0x40);
	ASSERT (_dg_nextpow (0x100) == 0x100);
	ASSERT (_dg_nextpow (0x0ff) == 0x100);
	ASSERT (_dg_nextpow (0x101) == 0x200);
	ASSERT (_dg_nextpow (0x400) == 0x400);
	ASSERT (_dg_nextpow (0x800) == 0x800);
	ASSERT (_dg_nextpow (0x700) == 0x800);
	ASSERT (_dg_nextpow (0x7ff) == 0x800);
	ASSERT (_dg_nextpow (0x801) == 0x1000);
	ASSERT (_dg_nextpow (0x40000000) == 0x40000000);
	ASSERT (_dg_nextpow (0x40007000) == 0x80000000);
	ASSERT (_dg_nextpow (0x80000000) == 0);

	PRINT ("test 3 ok\n");
}

void main4 (void)
{
	struct mynode nods[MAX];
	register int i;

	/* test for dg_test and dg_cmp on plain adjacency lists */

	ASSERT (MAX >= 10);
	for (i = 0; i < MAX; i++) {
		nods[i].data = i;
		dg_init (&nods[i].nod);
	}

	/* test must fail on an empty adj. list */
	ASSERT (dg_test (&nods[0].nod, &nods[1].nod) == 0);
	ASSERT (dg_test (&nods[0].nod, &nods[2].nod) == 0);
	ASSERT (dg_test (&nods[1].nod, &nods[0].nod) == 0);
	ASSERT (dg_test (&nods[2].nod, &nods[0].nod) == 0);

	/* cmp must succeed on two empty nodes */
	ASSERT (dg_cmp (&nods[0].nod, &nods[1].nod) == 0);

	/* define some edges from node 0: 0 -> 1;  0 -> 2;  0 -> 3 */
	dg_add (&nods[0].nod, &nods[1].nod);
	dg_add (&nods[0].nod, &nods[2].nod);
	dg_add (&nods[0].nod, &nods[3].nod);

	/* define some edges from node 1: 1 -> 2;  1 -> 3 */
	dg_add (&nods[1].nod, &nods[3].nod);
	dg_add (&nods[1].nod, &nods[2].nod);

	/* some positive tests */
	ASSERT (dg_test (&nods[0].nod, &nods[1].nod));
	ASSERT (dg_test (&nods[0].nod, &nods[3].nod));
	ASSERT (dg_test (&nods[0].nod, &nods[2].nod));

	ASSERT (dg_test (&nods[1].nod, &nods[3].nod));
	ASSERT (dg_test (&nods[1].nod, &nods[2].nod));

	/* some negative tests */
	ASSERT (dg_test (&nods[0].nod, &nods[6].nod) == 0);
	ASSERT (dg_test (&nods[0].nod, &nods[4].nod) == 0);

	ASSERT (dg_test (&nods[1].nod, &nods[4].nod) == 0);
	ASSERT (dg_test (&nods[1].nod, &nods[0].nod) == 0);
	ASSERT (dg_test (&nods[1].nod, &nods[1].nod) == 0);

	/* cmp must fail on two different nodes */
	ASSERT (dg_cmp (&nods[0].nod, &nods[5].nod) == 1);
	ASSERT (dg_cmp (&nods[5].nod, &nods[0].nod) == 1);
	ASSERT (dg_cmp (&nods[0].nod, &nods[1].nod) == 1);
	ASSERT (dg_cmp (&nods[1].nod, &nods[0].nod) == 1);

	/* if we add to node 1 an edge to node 1, then node 0 and node 1 are
	 * equal */
	dg_add (&nods[1].nod, &nods[1].nod);

	/* test for this condition */
	ASSERT (dg_cmp (&nods[0].nod, &nods[1].nod) == 0);
	ASSERT (dg_cmp (&nods[1].nod, &nods[0].nod) == 0);

	PRINT ("test 4 ok\n");
}

void main5 (void)
{
	struct mynode nods[MAX];
	register int i;

	/* test for dg_test and dg_cmp on plain adjacency lists */

	ASSERT (MAX >= 10);
	for (i = 0; i < MAX; i++) {
		nods[i].data = i;
		dg_init (&nods[i].nod);
	}

	/* initialize node 0 to point to node 2 */
	dg_add (&nods[0].nod, &nods[2].nod);

	/* node 0 and 1 must be different */
	ASSERT (dg_cmp (&nods[0].nod, &nods[1].nod) == 1);
	ASSERT (dg_cmp (&nods[1].nod, &nods[0].nod) == 1);

	/* copy node 0 to node 1 and append edge to node 3 */
	dg_cpy (&nods[1].nod, &nods[0].nod);
	dg_add (&nods[1].nod, &nods[3].nod);

	/* copy node 1 to node 2 and append edge to node 4 */
	dg_cpy (&nods[2].nod, &nods[1].nod);
	dg_add (&nods[2].nod, &nods[4].nod);

	/* copy node 2 to node 3 and append edge to node 5 */
	dg_cpy (&nods[3].nod, &nods[2].nod);
	dg_add (&nods[3].nod, &nods[5].nod);

	/* at this point, node 3 points to nodes 2 3 4 5; build a node with the
	 * same pointers in node 4 and compare */
	dg_add (&nods[4].nod, &nods[3].nod);
	dg_add (&nods[4].nod, &nods[5].nod);
	dg_add (&nods[4].nod, &nods[4].nod);
	dg_add (&nods[4].nod, &nods[2].nod);

	ASSERT (dg_test (&nods[3].nod, &nods[2].nod) == 1);
	ASSERT (dg_test (&nods[3].nod, &nods[3].nod) == 1);
	ASSERT (dg_test (&nods[3].nod, &nods[4].nod) == 1);
	ASSERT (dg_test (&nods[3].nod, &nods[5].nod) == 1);
	ASSERT (dg_test (&nods[3].nod, &nods[1].nod) == 0);
	ASSERT (dg_test (&nods[3].nod, &nods[6].nod) == 0);
	ASSERT (dg_test (&nods[3].nod, &nods[7].nod) == 0);

	ASSERT (dg_test (&nods[4].nod, &nods[2].nod) == 1);
	ASSERT (dg_test (&nods[4].nod, &nods[3].nod) == 1);
	ASSERT (dg_test (&nods[4].nod, &nods[4].nod) == 1);
	ASSERT (dg_test (&nods[4].nod, &nods[5].nod) == 1);
	ASSERT (dg_test (&nods[4].nod, &nods[1].nod) == 0);
	ASSERT (dg_test (&nods[4].nod, &nods[6].nod) == 0);
	ASSERT (dg_test (&nods[4].nod, &nods[7].nod) == 0);

	ASSERT (dg_cmp (&nods[3].nod, &nods[4].nod) == 0);
	ASSERT (dg_cmp (&nods[4].nod, &nods[3].nod) == 0);

	ASSERT (dg_cmp (&nods[2].nod, &nods[3].nod) == 1);
	ASSERT (dg_cmp (&nods[1].nod, &nods[3].nod) == 1);
	ASSERT (dg_cmp (&nods[0].nod, &nods[3].nod) == 1);

	ASSERT (dg_cmp (&nods[4].nod, &nods[1].nod) == 1);
	ASSERT (dg_cmp (&nods[4].nod, &nods[0].nod) == 1);

	PRINT ("test 5 ok\n");
}

void main4_2 (void)
{
	struct mynode nods[MAX];
	register int i;

	/* test for dg_test and dg_cmp on heap adjacency lists */

	ASSERT (MAX >= 10);
	for (i = 0; i < MAX; i++) {
		nods[i].data = i;
		dg_init (&nods[i].nod);
	}

	/* test must fail on an empty adj. list */
	ASSERT (dg_test2 (&nods[0].nod, &nods[1].nod) == 0);
	ASSERT (dg_test2 (&nods[0].nod, &nods[2].nod) == 0);
	ASSERT (dg_test2 (&nods[1].nod, &nods[0].nod) == 0);
	ASSERT (dg_test2 (&nods[2].nod, &nods[0].nod) == 0);

	/* cmp must succeed on two empty nodes */
	ASSERT (dg_cmp2 (&nods[0].nod, &nods[1].nod) == 0);

	/* define some edges from node 0: 0 -> 1;  0 -> 2;  0 -> 3 */
	dg_add2 (&nods[0].nod, &nods[3].nod);
	dg_add2 (&nods[0].nod, &nods[2].nod);
	dg_add2 (&nods[0].nod, &nods[1].nod);
	DEBUG ("After 0>3 0>2 0>1");
	db_adj2 (&nods[0].nod, nods);

	/* define some edges from node 1: 1 -> 2;  1 -> 3 */
	dg_add2 (&nods[1].nod, &nods[3].nod);
	dg_add2 (&nods[1].nod, &nods[2].nod);
	DEBUG ("After 1>2 1>3");
	db_adj2 (&nods[1].nod, nods);

	/* some positive tests */
	ASSERT (dg_test2 (&nods[0].nod, &nods[1].nod));
	ASSERT (dg_test2 (&nods[0].nod, &nods[3].nod));
	ASSERT (dg_test2 (&nods[0].nod, &nods[2].nod));

	ASSERT (dg_test2 (&nods[1].nod, &nods[3].nod));
	ASSERT (dg_test2 (&nods[1].nod, &nods[2].nod));

	/* some negative tests */
	ASSERT (dg_test2 (&nods[0].nod, &nods[6].nod) == 0);
	ASSERT (dg_test2 (&nods[0].nod, &nods[4].nod) == 0);

	ASSERT (dg_test2 (&nods[1].nod, &nods[4].nod) == 0);
	ASSERT (dg_test2 (&nods[1].nod, &nods[0].nod) == 0);
	ASSERT (dg_test2 (&nods[1].nod, &nods[1].nod) == 0);

	/* cmp must fail on two different nodes */
	ASSERT (dg_cmp2 (&nods[0].nod, &nods[5].nod) == 1);
	ASSERT (dg_cmp2 (&nods[5].nod, &nods[0].nod) == 1);
	ASSERT (dg_cmp2 (&nods[0].nod, &nods[1].nod) == 1);
	ASSERT (dg_cmp2 (&nods[1].nod, &nods[0].nod) == 1);

	/* if we add to node 1 an edge to node 1, then node 0 and node 1 are
	 * equal */
	dg_add2 (&nods[1].nod, &nods[1].nod);

	/* test for this condition */
	ASSERT (dg_cmp2 (&nods[0].nod, &nods[1].nod) == 0);
	ASSERT (dg_cmp2 (&nods[1].nod, &nods[0].nod) == 0);

	PRINT ("test 4_2 ok\n");
}

void main5_2 (void)
{
	struct mynode nods[MAX];
	register int i;

	/* test for dg_cpy2, dg_test2 and dg_cmp2 on heap adjacency lists */

	ASSERT (MAX >= 10);
	for (i = 0; i < MAX; i++) {
		nods[i].data = i;
		dg_init (&nods[i].nod);
	}

	/* initialize node 0 to point to node 2 */
	dg_add2 (&nods[0].nod, &nods[2].nod);

	/* node 0 and 1 must be different */
	ASSERT (dg_cmp2 (&nods[0].nod, &nods[1].nod) == 1);
	ASSERT (dg_cmp2 (&nods[1].nod, &nods[0].nod) == 1);

	/* copy node 0 to node 1 and append edge to node 3 */
	dg_cpy2 (&nods[1].nod, &nods[0].nod);
	dg_add2 (&nods[1].nod, &nods[3].nod);

	/* copy node 1 to node 2 and append edge to node 4 */
	dg_cpy2 (&nods[2].nod, &nods[1].nod);
	dg_add2 (&nods[2].nod, &nods[4].nod);

	/* copy node 2 to node 3 and append edge to node 5 */
	dg_cpy2 (&nods[3].nod, &nods[2].nod);
	dg_add2 (&nods[3].nod, &nods[5].nod);

	/* at this point, node 3 points to nodes 2 3 4 5; build a node with the
	 * same pointers in node 4 and compare */
	dg_add2 (&nods[4].nod, &nods[3].nod);
	dg_add2 (&nods[4].nod, &nods[4].nod);
	dg_add2 (&nods[4].nod, &nods[5].nod);
	dg_add2 (&nods[4].nod, &nods[2].nod);

	ASSERT (dg_test2 (&nods[3].nod, &nods[2].nod) == 1);
	ASSERT (dg_test2 (&nods[3].nod, &nods[3].nod) == 1);
	ASSERT (dg_test2 (&nods[3].nod, &nods[4].nod) == 1);
	ASSERT (dg_test2 (&nods[3].nod, &nods[5].nod) == 1);
	ASSERT (dg_test2 (&nods[3].nod, &nods[1].nod) == 0);
	ASSERT (dg_test2 (&nods[3].nod, &nods[6].nod) == 0);
	ASSERT (dg_test2 (&nods[3].nod, &nods[7].nod) == 0);

	ASSERT (dg_test2 (&nods[4].nod, &nods[2].nod) == 1);
	ASSERT (dg_test2 (&nods[4].nod, &nods[3].nod) == 1);
	ASSERT (dg_test2 (&nods[4].nod, &nods[4].nod) == 1);
	ASSERT (dg_test2 (&nods[4].nod, &nods[5].nod) == 1);
	ASSERT (dg_test2 (&nods[4].nod, &nods[1].nod) == 0);
	ASSERT (dg_test2 (&nods[4].nod, &nods[6].nod) == 0);
	ASSERT (dg_test2 (&nods[4].nod, &nods[7].nod) == 0);

	ASSERT (dg_cmp2 (&nods[3].nod, &nods[4].nod) == 0);
	ASSERT (dg_cmp2 (&nods[4].nod, &nods[3].nod) == 0);

	ASSERT (dg_cmp2 (&nods[2].nod, &nods[3].nod) == 1);
	ASSERT (dg_cmp2 (&nods[1].nod, &nods[3].nod) == 1);
	ASSERT (dg_cmp2 (&nods[0].nod, &nods[3].nod) == 1);

	ASSERT (dg_cmp2 (&nods[4].nod, &nods[1].nod) == 1);
	ASSERT (dg_cmp2 (&nods[4].nod, &nods[0].nod) == 1);

	DEBUG ("node 3:");
	db_adj2 (&nods[3].nod, nods);
	DEBUG ("node 4:");
	db_adj2 (&nods[4].nod, nods);
	PRINT ("test 5_2 ok\n");
}

void main6 (void)
{
	struct mynode nods[MAX];
	register int i;

	/* test for the heap adj lists */

	ASSERT (MAX >= 10);
	for (i = 0; i < MAX; i++) {
		nods[i].data = i;
		dg_init (&nods[i].nod);
	}

	db_adj2 (&nods[0].nod, nods);

	DEBUG ("add 0 1");
	dg_add2 (&nods[0].nod, &nods[1].nod);
	DEBUG ("add 0 3");
	dg_add2 (&nods[0].nod, &nods[3].nod);
	DEBUG ("add 0 2");
	dg_add2 (&nods[0].nod, &nods[2].nod);
	db_adj2 (&nods[0].nod, nods);

	DEBUG ("add 0 4");
	dg_add2 (&nods[0].nod, &nods[4].nod);
	db_adj2 (&nods[0].nod, nods);
	PRINT ("end of main6\n");
}

void main7 (void)
{
	struct mynode nods[MAX];
	register int i;

	/* test for the heap adj lists: dg_rem2 */

	ASSERT (MAX >= 10);
	for (i = 0; i < MAX; i++) {
		nods[i].data = i;
		dg_init (&nods[i].nod);
	}

	/* 0>1 0>2 0>3 0>4 */
	dg_add2 (&nods[0].nod, &nods[3].nod);
	dg_add2 (&nods[0].nod, &nods[1].nod);
	dg_add2 (&nods[0].nod, &nods[4].nod);
	dg_add2 (&nods[0].nod, &nods[2].nod);

	/* 1>1 1>2 1>3 1>4 */
	dg_add2 (&nods[1].nod, &nods[2].nod);
	dg_add2 (&nods[1].nod, &nods[3].nod);
	dg_add2 (&nods[1].nod, &nods[4].nod);
	dg_add2 (&nods[1].nod, &nods[1].nod);

	/* cmp must succeed between nodes 0 and 1 */
	ASSERT (dg_cmp2 (&nods[0].nod, &nods[1].nod) == 0);
	ASSERT (dg_cmp2 (&nods[1].nod, &nods[0].nod) == 0);

	/* and also (always!) between the same node */
	ASSERT (dg_cmp2 (&nods[1].nod, &nods[1].nod) == 0);
	ASSERT (dg_cmp2 (&nods[0].nod, &nods[0].nod) == 0);

	/* 2>2 2>3 2>4 */
	dg_add2 (&nods[2].nod, &nods[4].nod);
	dg_add2 (&nods[2].nod, &nods[3].nod);
	dg_add2 (&nods[2].nod, &nods[2].nod);

	/* cmp must not succeed between nodes 1 and 2, or nodes 0 and 2 */
	ASSERT (dg_cmp2 (&nods[1].nod, &nods[2].nod) != 0);
	ASSERT (dg_cmp2 (&nods[2].nod, &nods[1].nod) != 0);
	ASSERT (dg_cmp2 (&nods[2].nod, &nods[0].nod) != 0);
	ASSERT (dg_cmp2 (&nods[0].nod, &nods[2].nod) != 0);

	/* and also (always!) between the same node */
	ASSERT (dg_cmp2 (&nods[1].nod, &nods[1].nod) == 0);
	ASSERT (dg_cmp2 (&nods[0].nod, &nods[0].nod) == 0);

	/* remove edge 1>1 from node 1 */
	dg_rem2 (&nods[1].nod, &nods[1].nod);

	/* cmp must succeed between nodes 1 and 2 */
	ASSERT (dg_cmp2 (&nods[2].nod, &nods[1].nod) == 0);
	ASSERT (dg_cmp2 (&nods[1].nod, &nods[2].nod) == 0);

	/* add again edge 1>1 and compare */
	dg_add2 (&nods[1].nod, &nods[1].nod);
	ASSERT (dg_cmp2 (&nods[2].nod, &nods[1].nod) != 0);
	ASSERT (dg_cmp2 (&nods[1].nod, &nods[2].nod) != 0);

	/* remove all edges but 1>4 */
	while (1) {
		for (i = 1; i <= nods[1].nod.deg; i++) {
			if (nods[1].nod.adj[i] != &nods[4].nod) break;
		}
		if (i > nods[1].nod.deg) break;
		dg_rem2 (&nods[1].nod, nods[1].nod.adj[i]);
	}
	ASSERT (nods[1].nod.deg == 1);
	ASSERT (nods[1].nod.adj[1] == &nods[4].nod);

	/* cmp must not succeed between nodes 1 and 2 */
	ASSERT (dg_cmp2 (&nods[1].nod, &nods[2].nod) != 0);
	ASSERT (dg_cmp2 (&nods[2].nod, &nods[1].nod) != 0);

	/* remove edge 1>4 */
	dg_rem2 (&nods[1].nod, &nods[4].nod);

	/* cmp must succeed with other empty node */
	ASSERT (dg_cmp2 (&nods[1].nod, &nods[5].nod) == 0);
	ASSERT (dg_cmp2 (&nods[5].nod, &nods[1].nod) == 0);

	/* now add edges 0>1 0>2 0>3 0>4 */
	dg_add2 (&nods[1].nod, &nods[3].nod);
	dg_add2 (&nods[1].nod, &nods[1].nod);
	dg_add2 (&nods[1].nod, &nods[4].nod);
	dg_add2 (&nods[1].nod, &nods[2].nod);

	/* cmp must succeed between nodes 1 and 0 */
	ASSERT (dg_cmp2 (&nods[0].nod, &nods[1].nod) == 0);
	ASSERT (dg_cmp2 (&nods[1].nod, &nods[0].nod) == 0);
	PRINT ("test 7 ok\n");
}

int main (void)
{
	// main1 ();
	main3 ();

	main6 ();
	main7 ();

	main4 ();
	main5 ();
	main4_2 ();
	main5_2 ();
	return 0;
}

