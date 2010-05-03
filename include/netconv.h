
#ifndef _NETCONV_H_
#define _NETCONV_H_

#include "global.h"
#include "dg.h"

void nc_create_net (void);
void nc_create_unfolding (void);
struct place * nc_create_place (void);
struct trans * nc_create_transition (void);
void nc_create_arc (struct dg * src_post, struct dg * dst_pre,
		struct dg * src_pre, struct dg * dst_post);
void nc_compute_sizes (void);
void nc_static_checks (const char * stoptr);

#endif
