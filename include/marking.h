
#ifndef _MARKING_H_
#define _MARKING_H_

#include "nodelist.h"
#include "h.h"

void marking_init (void);
int marking_find (const struct h *h);
void marking_add (struct h *h);
void marking_print (const struct h *h);

#endif

