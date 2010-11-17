
/*
 * Adjacency List -- interface
 */

#ifndef _AL_AL_H_
#define _AL_AL_H_

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

#endif

