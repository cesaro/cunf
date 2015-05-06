
/* 
 * Copyright (C) 2010, 2011  Cesar Rodriguez <cesar.rodriguez@lsv.ens-cachan.fr>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _UTIL_GLUE_H_
#define _UTIL_GLUE_H_

#include <stdint.h>

#include "util/config.h"
#include "util/misc.h"

#ifdef __cplusplus
extern "C" {
#endif

void ut_free (void * ptr);
void * ut_malloc (int size);
void * ut_realloc (void * ptr, int size);
char * ut_strdup (char * str);

void ut_err (const char * fmt, ...);
void ut_warn (const char * fmt, ...);

uint64_t ut_get_max_rss   (void);
int      ut_set_mem_limit (uint64_t max_mem_mb);
int      ut_set_cpu_limit (uint64_t max_time_sec);


#ifdef __cplusplus // extern C
}
#endif

#endif

