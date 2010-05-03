
#ifndef _PE_H_
#define _PE_H_

#include "global.h"
#include "h.h"

void pe_init (void);
void pe_term (void);

void pe_update (struct h * h);
struct h * pe_pop (void);

#endif

