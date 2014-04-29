
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

#ifndef _CUNF_DEBUG_H_
#define _CUNF_DEBUG_H_

#include "cunf/global.h"
#include "cunf/h.h"
#include "cunf/ec.h"

#ifdef __cplusplus
extern "C" {
#endif

void db_net (void);
void db_e (struct event *e);
void db_c (struct cond *c);
void db_h (struct h *h);
void db_r (struct ec *r);
void db_r2 (const char *str1, struct ec *r, const char *str2);
void db_hgraph (void);
void db_h2dot (void);
void db_mem (void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif

