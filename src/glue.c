
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <stdarg.h>

#include "debug.h"
#include "glue.h"

void gl_free (void * ptr)
{
	free (ptr);
}

void * gl_malloc (int size)
{
	void * ptr;

	ptr = malloc (size);
	if (ptr == 0) gl_err ("No more memory");
	DEBUG ("ptr=%p size=%d", ptr, size);
	return ptr;
}

void * gl_realloc (void * ptr, int size)
{
	void * ptr2;

	ptr2 = realloc (ptr, size);
	if (ptr2 == 0) gl_err ("No more memory");
	DEBUG ("ptr=%p size=%d ptr2=%p", ptr, size, ptr2);
	return ptr2;
}

char * gl_strdup (char * str)
{
	char * s;

	s = strdup (str);
	if (! s) gl_err ("No more memory");
	return s;
}

void gl_err (const char * fmt, ...)
{
	va_list	args;

	va_start (args, fmt);
	verrx (EXIT_FAILURE, fmt, args);
}

void gl_warn (const char * fmt, ...)
{
	va_list	args;

	va_start (args, fmt);
	vwarnx (fmt, args);
	va_end (args);
}

