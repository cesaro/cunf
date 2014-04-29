
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

#include "util/config.h"
#include "util/misc.h"

#ifdef __cplusplus
extern "C" {
#endif

void gl_free (void * ptr);
void * gl_malloc (int size);
void * gl_realloc (void * ptr, int size);
char * gl_strdup (char * str);

void gl_err (const char * fmt, ...);
void gl_warn (const char * fmt, ...);

#ifdef __cplusplus // extern C
}
#endif

#endif

