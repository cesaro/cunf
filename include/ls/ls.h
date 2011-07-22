
/*
 * Static Singly-Linked List -- interface
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

#ifndef _LS_LS_H_
#define _LS_LS_H_

struct ls {
	struct ls * next;
};

void ls_init (struct ls * l);
void ls_insert (struct ls * l, struct ls * n);
void ls_append (struct ls * l, struct ls * n);
void ls_remove (struct ls * l, struct ls * p);
void ls_shift (struct ls * l);
void ls_reverse (struct ls * l);
int ls_iter (struct ls * l, int (* callback) (struct ls * n));
void ls_print (struct ls * l, const char * heading);

void ls_push (struct ls * l, struct ls *n);
struct ls * ls_pop (struct ls * l);

#define ls_item(type,nod,field) \
		((type *) ((void *) (nod) - (void *) &((type *) 0)->field))

#define ls_i ls_item

#endif

