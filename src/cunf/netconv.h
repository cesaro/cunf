
#ifndef _CUNF_NETCONV_H_
#define _CUNF_NETCONV_H_

#include "cunf/global.h"
#include "util/al.h"

#ifdef __cplusplus
extern "C" {
#endif

void nc_create_net (void);
void nc_create_unfolding (void);
struct place * nc_create_place (void);
struct trans * nc_create_transition (void);
int nc_create_arc (struct al * src_post, struct al * dst_pre,
		void * src, void * dst);
void nc_static_checks (void);

#ifdef __cplusplus // extern C
}
#endif

#endif
