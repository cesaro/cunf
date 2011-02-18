
/*
 * Static Singly-Linked List with Tail -- interface
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

#ifndef _LST_LST_H_
#define _LST_LST_H_

struct lst {
	struct lstn * head;
	struct lstn * tail;
};

struct lstn {
	struct lstn * next;
};

void lst_init (struct lst * l);
void lst_insert (struct lst * l, struct lstn * n);
void lst_append (struct lst * l, struct lstn * n);
void lst_remove (struct lst * l, struct lstn * p);
void lst_shift (struct lst * l);
int lst_iter (struct lst * l, int (* callback) (struct lstn * n));
void lst_print (struct lst * l, const char * heading);

void lst_push (struct lst * l, struct lstn *n);
struct lstn * lst_pop (struct lst * l);

#define lst_item(type,nod,field) \
		((type *) ((void *) (nod) - (void *) &((type *) 0)->field))

#define lst_i lst_item

#endif

