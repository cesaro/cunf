
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

#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <stdarg.h>
#include <sys/resource.h>

#include "util/misc.h"
#include "util/system.h"

#if defined(__linux__)
static uint64_t __linux_get_max_rss (void);
#elif defined(__APPLE__)
static uint64_t __apple_get_malloc_max (void);
#endif
static int __ut_set_rlimit (int what, uint64_t max);

void ut_free (void * ptr)
{
	free (ptr);
}

void * ut_malloc (int size)
{
	void * ptr;

	ptr = malloc (size);
	// DEBUG ("ptr=%p size=%d", ptr, size);
	if (ptr == 0 && size) ut_err ("No more memory");
	return ptr;
}

void * ut_realloc (void * ptr, int size)
{
	void * ptr2;

	ptr2 = realloc (ptr, size);
	// DEBUG ("ptr=%p size=%d ptr2=%p", ptr, size, ptr2);
	if (ptr2 == 0 && size) ut_err ("No more memory");
	return ptr2;
}

char * ut_strdup (char * str)
{
	char * s;

	s = strdup (str);
	if (! s) ut_err ("No more memory");
	return s;
}

void ut_err (const char * fmt, ...)
{
	va_list	args;

	va_start (args, fmt);
	verrx (EXIT_FAILURE, fmt, args);
}

void ut_warn (const char * fmt, ...)
{
	va_list	args;

	va_start (args, fmt);
	vwarnx (fmt, args);
	va_end (args);
}

/* returns the maximum RSS size in megabytes */
uint64_t ut_get_max_rss (void)
{
#if defined(__linux__)
	return __linux_get_max_rss ();
#elif defined(__APPLE__)
	return __apple_get_malloc_max ();
#else
#warn Unsupported OS, ut_get_max_rss will yield incorrect result
	/* FIXME, can we use getresusage(2) here? */
	return 0;
#endif
}

/* sets the maximum memory limit in megabytes */
int ut_set_mem_limit (uint64_t max_mem_mb)
{
	return __ut_set_rlimit (RLIMIT_DATA, max_mem_mb << 10);
}

/* sets the maximum cpu time in seconds, receives signal SIGXCPU when exceded */
int ut_set_cpu_limit (uint64_t max_time_sec)
{
	return __ut_set_rlimit (RLIMIT_CPU, max_time_sec);
}

static int __ut_set_rlimit (int what, uint64_t max)
{
	struct rlimit rl;
	int ret;

	if (max == 0) return 0;

	/* FIXME -- check if this works in linux */
	ret = getrlimit (what, &rl);
	if (ret < 0) return ret;
	if (rl.rlim_max != RLIM_INFINITY && max > rl.rlim_max) return -1;

	rl.rlim_cur = max;
	return setrlimit (what, &rl);
}


/*************** What follows is architecture dependent code *************/

#if defined(__linux__)
static uint64_t __linux_get_max_rss (void)
{
	fd = open ("/proc/self/statm", O_RDONLY);
	if (fd < 0) return;
	ret = read (fd, buff, 128);
	close (fd);
	buff[127] = 0;
	return strtoul (buff, 0, 10) * sysconf (_SC_PAGESIZE) >> 10;
}

#elif defined(__APPLE__)

#include <malloc/malloc.h>

static uint64_t __apple_get_malloc_max (void)
{
    malloc_statistics_t t;
    malloc_zone_statistics(NULL, &t);
    return t.max_size_in_use >> 10;
}
#endif

