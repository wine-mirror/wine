/*
 * msvcrt.dll misc functions
 *
 * Copyright 2000 Jon Griffiths
 */

#include "msvcrt.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);

typedef int (__cdecl *MSVCRT_comp_func)(const void*, const void*);

/*********************************************************************
 *		_beep (MSVCRT.@)
 */
void __cdecl MSVCRT__beep( unsigned int freq, unsigned int duration)
{
    TRACE(":Freq %d, Duration %d\n",freq,duration);
    Beep(freq, duration);
}

extern int rand(void);

/*********************************************************************
 *		rand (MSVCRT.@)
 */
int __cdecl MSVCRT_rand()
{
  return (rand() & 0x7fff);
}

/*********************************************************************
 *		_sleep (MSVCRT.@)
 */
void __cdecl MSVCRT__sleep(unsigned long timeout)
{
  TRACE("MSVCRT__sleep for %ld milliseconds\n",timeout);
  Sleep((timeout)?timeout:1);
}

/*********************************************************************
 *		_lfind (MSVCRT.@)
 */
void* __cdecl MSVCRT__lfind(const void * match, const void * start,
unsigned int * array_size,unsigned int elem_size, MSVCRT_comp_func cf)
{
  unsigned int size = *array_size;
  if (size)
    do
    {
      if (cf(match, start) == 0)
        return (void *)start; /* found */
      start += elem_size;
    } while (--size);
  return NULL;
}

/*********************************************************************
 *		_lsearch (MSVCRT.@)
 */
void * __cdecl MSVCRT__lsearch(const void * match,void *  start,
unsigned int * array_size,unsigned int elem_size, MSVCRT_comp_func cf)
{
  unsigned int size = *array_size;
  if (size)
    do
    {
      if (cf(match, start) == 0)
        return start; /* found */
      start += elem_size;
    } while (--size);

  /* not found, add to end */
  memcpy(start, match, elem_size);
  array_size[0]++;
  return start;
}

/*********************************************************************
 *		_chkesp (MSVCRT.@)
 */
void __cdecl MSVCRT__chkesp(void)
{

}
