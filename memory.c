/*
If the size is not stored in the hidden header, or it's stored in unexpected
address, it go wrong.  So this is a dirty HACK.
*/

#include "edef.h"
#include <stdlib.h>

void*	malloc(unsigned long);
void	free(void *);

#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

static inline size_t
allocated_size(void *p)
{
	if (p)
		return *((size_t *)p - 1) & ~1;	/* LSB -> 0 after `free` */
	else
		return 0;
}

#pragma GCC diagnostic ignored "-Warray-bounds"

void*
allocate(unsigned long nbytes)
{
#undef malloc
	char *mp = malloc(nbytes);
	envram += allocated_size(mp);
	return mp;
}

void
release(void *mp)
{
#undef free
	envram -= allocated_size(mp);
	free(mp);
}

#ifndef __clang__
#pragma GCC diagnostic pop
#endif
