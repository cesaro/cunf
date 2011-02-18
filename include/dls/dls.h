
/*
 * Static Doubly-Linked List
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

#ifndef _DLS_DLS_H_
#define _DLS_DLS_H_

struct dls {
	struct dls * prev;
	struct dls * next;
};

void dls_init (struct dls * l);
void dls_insert (struct dls * l, struct dls * n);
void dls_append (struct dls * l, struct dls * n);
void dls_remove (struct dls * l, struct dls * n);
void dls_shift (struct dls * l);
int dls_iter (struct dls * l, int (* callback) (struct dls * n));
void dls_print (struct dls * l, const char * heading);

void dls_push (struct dls * l, struct dls * n);
struct dls * dls_pop (struct dls * l);

#define dls_item(type,nod,field) \
		((type *) ((void *) (nod) - (void *) &((type *) 0)->field))

#define dls_i dls_item

#endif

