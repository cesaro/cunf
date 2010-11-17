
#ifndef _GLUE_H_
#define _GLUE_H_

#include "config.h"
#include "debug.h"

void gl_free (void * ptr);
void * gl_malloc (int size);
void * gl_realloc (void * ptr, int size);
char * gl_strdup (char * str);

void gl_err (const char * fmt, ...);
void gl_warn (const char * fmt, ...);

#endif

