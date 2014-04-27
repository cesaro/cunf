
/*
 * Adjacency List -- interface
 * 
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

#ifndef _UTIL_AL_H_
#define _UTIL_AL_H_

#ifdef __cplusplus
extern "C" {
#endif

struct al {
	int deg;
	void ** adj;
};

void al_init (struct al * n);
void al_term (struct al * n);

void al_add (struct al * n, void * ptr);
void al_rem (struct al * n, const void * ptr);
void al_cpy (struct al * dst, const struct al * src);
int al_test (const struct al * n, const void * ptr);
int al_cmp (const struct al * n1, const struct al * n2);

#ifdef __cplusplus // extern C
}
#endif

#endif

