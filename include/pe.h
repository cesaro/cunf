
#ifndef _PE_H_
#define _PE_H_

#include "ec.h"
#include "h.h"

void pe_init (void);
void pe_term (void);

void pe_update_gen (struct ec * r);
void pe_update_read (struct ec * r);
struct h * pe_pop (void);

#endif

