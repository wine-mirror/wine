#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
#define size_t unsigned int
#endif

void *xmalloc (size_t);
void *xrealloc (void *, size_t);
