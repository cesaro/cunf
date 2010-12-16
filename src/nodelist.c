
#include <stdlib.h>

#include "config.h"
#include "glue.h"
#include "nodelist.h"

/*****************************************************************************/
/* Allocate a nodelist element. To reduce memory fragmentation, struct nl    */
/* elements are allocated in large contingents, and freed cells are kept in  */
/* li_freelist. New allocation requests are served from li_freelist first.   */

typedef struct contingent_t
{
	struct nl *nodes;
	struct contingent_t *next;
} contingent_t;

struct nl *li_freelist = NULL;	  /* pointer to freed nodelist elements */
contingent_t *li_contlist = NULL; /* pointer to allocated contingents   */
int li_counter = 0;    /* num of free elements in the latest contingent */

static struct nl * _nl_alloc ()
{
	struct nl *tmp_nl;
	contingent_t *tmp_co;

	if ((tmp_nl = li_freelist))
	{
		li_freelist = tmp_nl->next;
		return tmp_nl;
	}

	if (li_counter--) return li_contlist->nodes + li_counter;

	tmp_co = gl_malloc (sizeof(contingent_t));
	tmp_co->nodes = gl_malloc (CONFIG_NODELIST_STEP * sizeof(struct nl));
	tmp_co->next = li_contlist;
	li_contlist = tmp_co;
	li_counter = CONFIG_NODELIST_STEP - 1;

	return li_contlist->nodes + li_counter;
}

/*****************************************************************************/
/* add a node to the front of a list					     */

struct nl* nl_push (struct nl **list, void *node)
{
	struct nl *newlist = _nl_alloc();
	newlist->node = node;
	newlist->next = *list;
	return *list = newlist;
}

/*****************************************************************************/
/* add a place to a list; sort by pointer in ascending order		     */

struct nl* nl_insert (struct nl **list, void *node)
{
	while (*list && node > (*list)->node)
		list = &((*list)->next);

	if (*list && (*list)->node == node) return *list;

	return nl_push(list,node);
}

/*****************************************************************************/
/* add a place to a list; sort using function cmp, which is expected to return
 * a negative value, 0 or a positive value if n1 < n2, n1 == n2 or n1 > n2 */

struct nl* nl_insert2 (struct nl **list, void *node,
		int (* cmp) (const void *n1, const void *n2))
{
	while (*list && cmp (node, (*list)->node) > 0) list = &((*list)->next);

	if (*list && cmp ((*list)->node, node) == 0) return *list;

	return nl_push (list,node);
}

/*****************************************************************************/
/* release a nodelist; move its cells to the "free" list		     */

void nl_delete (struct nl *list)
{
	struct nl *tmp;

	while (list)
	{
		tmp = list;
		list = list->next;
		tmp->next = li_freelist;
		li_freelist = tmp;
	}
}

/*****************************************************************************/
/* compare two lists; return 0 if equal, -1 if list1 < list2, 1 otherwise    */

char nl_compare (const struct nl *list1, const struct nl *list2)
{
	while (list1 && list2 && list1->node == list2->node)
		list1 = list1->next, list2 = list2->next;

	if (!list1 && !list2) return 0;
	if (!list1 || (list2 && list1->node < list2->node)) return -1;
	return 1;
}

