
#ifndef _NETCONV_H_
#define _NETCONV_H_

#include "nodelist.h"

void nc_create_net (void);
void nc_create_unfolding (void);
struct place * nc_create_place (void);
struct trans * nc_create_transition (void);
void nc_create_arc (struct nl **, struct nl **, void *, void *);
void nc_compute_sizes (void);
void nc_static_checks (void);

#endif
