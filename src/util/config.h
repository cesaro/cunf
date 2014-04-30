
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

/* see src/util/misc.h for more info */
#define CONFIG_MAX_VERB_LEVEL 3

/* test and debug */
#define CONFIG_DEBUG

/* unfold order */
#undef CONFIG_MCMILLAN
#define CONFIG_ERV
#undef CONFIG_ERV_MOLE
#undef CONFIG_PARIKH

/* see src/nodelist.c */
#define CONFIG_NODELIST_STEP 1024

/* experimental */
#undef CONFIG_PMASK

