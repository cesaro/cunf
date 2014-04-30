
#include "util/misc.h"

int __log_level = 3;

void log_set_level (int i)
{
	__log_level = i;
}

int log_get_level ()
{
	return __log_level;
}

void breakme (void)
{
}

