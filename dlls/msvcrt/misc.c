/*
 * msvcrt.dll misc functions
 *
 * Copyright 2000 Jon Griffiths
 */

#include <stdlib.h>

#include "msvcrt.h"
#include "msvcrt/stdlib.h"


#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);


/*********************************************************************
 *		_beep (MSVCRT.@)
 */
void _beep( unsigned int freq, unsigned int duration)
{
    TRACE(":Freq %d, Duration %d\n",freq,duration);
    Beep(freq, duration);
}

/*********************************************************************
 *		rand (MSVCRT.@)
 */
int MSVCRT_rand()
{
  return (rand() & 0x7fff);
}

/*********************************************************************
 *		_sleep (MSVCRT.@)
 */
void _sleep(unsigned long timeout)
{
  TRACE("_sleep for %ld milliseconds\n",timeout);
  Sleep((timeout)?timeout:1);
}

/*********************************************************************
 *		_lfind (MSVCRT.@)
 */
void* _lfind(const void* match, const void* start,
             unsigned int* array_size, unsigned int elem_size,
             MSVCRT_compar_fn_t cf)
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
void* _lsearch(const void* match, void* start,
               unsigned int* array_size, unsigned int elem_size,
               MSVCRT_compar_fn_t cf)
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
void _chkesp(void)
{

}
