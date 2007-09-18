/*
 * msvcrt.dll misc functions
 *
 * Copyright 2000 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>

#include "msvcrt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);


/*********************************************************************
 *		_beep (MSVCRT.@)
 */
void CDECL _beep( unsigned int freq, unsigned int duration)
{
    TRACE(":Freq %d, Duration %d\n",freq,duration);
    Beep(freq, duration);
}

/*********************************************************************
 *		srand (MSVCRT.@)
 */
void CDECL MSVCRT_srand( unsigned int seed )
{
    thread_data_t *data = msvcrt_get_thread_data();
    data->random_seed = seed;
}

/*********************************************************************
 *		rand (MSVCRT.@)
 */
int CDECL MSVCRT_rand(void)
{
    thread_data_t *data = msvcrt_get_thread_data();

    /* this is the algorithm used by MSVC, according to
     * http://en.wikipedia.org/wiki/List_of_pseudorandom_number_generators */
    data->random_seed = data->random_seed * 214013 + 2531011;
    return (data->random_seed >> 16) & MSVCRT_RAND_MAX;
}

/*********************************************************************
 *		_sleep (MSVCRT.@)
 */
void CDECL MSVCRT__sleep(unsigned long timeout)
{
  TRACE("_sleep for %ld milliseconds\n",timeout);
  Sleep((timeout)?timeout:1);
}

/*********************************************************************
 *		_lfind (MSVCRT.@)
 */
void* CDECL _lfind(const void* match, const void* start,
                   unsigned int* array_size, unsigned int elem_size,
                   int (*cf)(const void*,const void*) )
{
  unsigned int size = *array_size;
  if (size)
    do
    {
      if (cf(match, start) == 0)
        return (void *)start; /* found */
      start = (const char *)start + elem_size;
    } while (--size);
  return NULL;
}

/*********************************************************************
 *		_lsearch (MSVCRT.@)
 */
void* CDECL _lsearch(const void* match, void* start,
                     unsigned int* array_size, unsigned int elem_size,
                     int (*cf)(const void*,const void*) )
{
  unsigned int size = *array_size;
  if (size)
    do
    {
      if (cf(match, start) == 0)
        return start; /* found */
      start = (char*)start + elem_size;
    } while (--size);

  /* not found, add to end */
  memcpy(start, match, elem_size);
  array_size[0]++;
  return start;
}

/*********************************************************************
 *		_chkesp (MSVCRT.@)
 *
 * Trap to a debugger if the value of the stack pointer has changed.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Does not return.
 *
 * NOTES
 *  This function is available for iX86 only.
 *
 *  When VC++ generates debug code, it stores the value of the stack pointer
 *  before calling any external function, and checks the value following
 *  the call. It then calls this function, which will trap if the values are
 *  not the same. Usually this means that the prototype used to call
 *  the function is incorrect.  It can also mean that the .spec entry has
 *  the wrong calling convention or parameters.
 */
#ifdef __i386__

# ifdef __GNUC__

__ASM_GLOBAL_FUNC(_chkesp,
                  "jnz 1f\n\t"
                  "ret\n"
                  "1:\tpushl %ebp\n\t"
                  "movl %esp,%ebp\n\t"
                  "subl $12,%esp\n\t"
                  "pushl %eax\n\t"
                  "pushl %ecx\n\t"
                  "pushl %edx\n\t"
                  "call " __ASM_NAME("MSVCRT_chkesp_fail") "\n\t"
                  "popl %edx\n\t"
                  "popl %ecx\n\t"
                  "popl %eax\n\t"
                  "leave\n\t"
                  "ret")

void CDECL MSVCRT_chkesp_fail(void)
{
  ERR("Stack pointer incorrect after last function call - Bad prototype/spec entry?\n");
  DebugBreak();
}

# else  /* __GNUC__ */

/**********************************************************************/

void CDECL _chkesp(void)
{
}

# endif  /* __GNUC__ */

#endif  /* __i386__ */
