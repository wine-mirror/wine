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
#include "ntsecapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

static unsigned int output_format;

/*********************************************************************
 *		_beep (MSVCRT.@)
 */
void CDECL MSVCRT__beep( unsigned int freq, unsigned int duration)
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
 *		rand_s (MSVCRT.@)
 */
int CDECL MSVCRT_rand_s(unsigned int *pval)
{
    if (!pval || !RtlGenRandom(pval, sizeof(*pval)))
    {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }
    return 0;
}

/*********************************************************************
 *		_sleep (MSVCRT.@)
 */
void CDECL MSVCRT__sleep(MSVCRT_ulong timeout)
{
  TRACE("_sleep for %d milliseconds\n",timeout);
  Sleep((timeout)?timeout:1);
}

/*********************************************************************
 *		_lfind (MSVCRT.@)
 */
void* CDECL _lfind(const void* match, const void* start,
                   unsigned int* array_size, unsigned int elem_size,
                   int (CDECL *cf)(const void*,const void*) )
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
                     int (CDECL *cf)(const void*,const void*) )
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
 *                  bsearch_s (msvcrt.@)
 */
void* CDECL MSVCRT_bsearch_s(const void *key, const void *base,
                             MSVCRT_size_t nmemb, MSVCRT_size_t size,
                             int (__cdecl *compare)(void *, const void *, const void *), void *ctx)
{
    ssize_t min = 0;
    ssize_t max = nmemb - 1;

    if (!MSVCRT_CHECK_PMT(size != 0)) return NULL;
    if (!MSVCRT_CHECK_PMT(compare != NULL)) return NULL;

    while (min <= max)
    {
        ssize_t cursor = (min + max) / 2;
        int ret = compare(ctx, key,(const char *)base+(cursor*size));
        if (!ret)
            return (char*)base+(cursor*size);
        if (ret < 0)
            max = cursor - 1;
        else
            min = cursor + 1;
    }
    return NULL;
}

static int CDECL compare_wrapper(void *ctx, const void *e1, const void *e2)
{
    int (__cdecl *compare)(const void *, const void *) = ctx;
    return compare(e1, e2);
}

/*********************************************************************
 *                  bsearch (msvcrt.@)
 */
void* CDECL MSVCRT_bsearch(const void *key, const void *base, MSVCRT_size_t nmemb,
        MSVCRT_size_t size, int (__cdecl *compar)(const void *, const void *))
{
    return MSVCRT_bsearch_s(key, base, nmemb, size, compare_wrapper, compar);
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
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                  "movl %esp,%ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                  "subl $12,%esp\n\t"
                  "pushl %eax\n\t"
                  "pushl %ecx\n\t"
                  "pushl %edx\n\t"
                  "call " __ASM_NAME("MSVCRT_chkesp_fail") "\n\t"
                  "popl %edx\n\t"
                  "popl %ecx\n\t"
                  "popl %eax\n\t"
                  "leave\n\t"
                  __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                  __ASM_CFI(".cfi_same_value %ebp\n\t")
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

/*********************************************************************
 * Helper function for MSVCRT_qsort_s.
 *
 * Based on NTDLL_qsort in dlls/ntdll/misc.c
 */
static void MSVCRT_mergesort( void *arr, void *barr, size_t elemsize,
        int (CDECL *compar)(void *, const void *, const void *),
        size_t left, size_t right, void *context )
{
    if (right>left) {
        size_t i, j, k, m;
        m=left+(right-left)/2;
        MSVCRT_mergesort(arr, barr, elemsize, compar, left, m, context);
        MSVCRT_mergesort(arr, barr, elemsize, compar, m+1, right, context);

#define X(a,i) ((char*)a+elemsize*(i))
        for (i=m+1; i>left; i--)
            memcpy (X(barr,(i-1)),X(arr,(i-1)),elemsize);
        for (j=m; j<right; j++)
            memcpy (X(barr,(right+m-j)),X(arr,(j+1)),elemsize);

        /* i=left; j=right; */
        for (k=left; i<=m && j>m; k++) {
            if (i==j || compar(context, X(barr,i),X(barr,j))<=0) {
                memcpy(X(arr,k),X(barr,i),elemsize);
                i++;
            } else {
                memcpy(X(arr,k),X(barr,j),elemsize);
                j--;
            }
        }
        for (; i<=m; i++, k++)
            memcpy(X(arr,k),X(barr,i),elemsize);
        for (; j>m; j--, k++)
            memcpy(X(arr,k),X(barr,j),elemsize);
    }
#undef X
}

/*********************************************************************
 * qsort_s (MSVCRT.@)
 *
 * Based on NTDLL_qsort in dlls/ntdll/misc.c
 */
void CDECL MSVCRT_qsort_s(void *base, MSVCRT_size_t nmemb, MSVCRT_size_t size,
    int (CDECL *compar)(void *, const void *, const void *), void *context)
{
    void *secondarr;
    const size_t total_size = nmemb*size;

    if (!MSVCRT_CHECK_PMT(base != NULL || (base == NULL && nmemb == 0))) return;
    if (!MSVCRT_CHECK_PMT(size > 0)) return;
    if (!MSVCRT_CHECK_PMT(compar != NULL)) return;
    if (total_size / size != nmemb) return;

    if (nmemb < 2) return;

    secondarr = MSVCRT_malloc(total_size);
    if (!secondarr)
        return;
    MSVCRT_mergesort(base, secondarr, size, compar, 0, nmemb-1, context);
    MSVCRT_free(secondarr);
}

/*********************************************************************
 * qsort (MSVCRT.@)
 */
void CDECL MSVCRT_qsort(void *base, MSVCRT_size_t nmemb, MSVCRT_size_t size,
        int (CDECL *compar)(const void*, const void*))
{
    return MSVCRT_qsort_s(base, nmemb, size, compare_wrapper, compar);
}

/*********************************************************************
 * _get_output_format (MSVCRT.@)
 */
unsigned int CDECL MSVCRT__get_output_format(void)
{
   return output_format;
}

/*********************************************************************
 * _set_output_format (MSVCRT.@)
 */
unsigned int CDECL _set_output_format(unsigned int new_output_format)
{
    unsigned int ret = output_format;

    if(!MSVCRT_CHECK_PMT(new_output_format==0 || new_output_format==MSVCRT__TWO_DIGIT_EXPONENT))
        return ret;

    output_format = new_output_format;
    return ret;
}

/*********************************************************************
 * _resetstkoflw (MSVCRT.@)
 */
int CDECL MSVCRT__resetstkoflw(void)
{
    int stack_addr;

    /* causes stack fault that updates NtCurrentTeb()->Tib.StackLimit */
    return VirtualProtect( &stack_addr, 1, PAGE_GUARD|PAGE_READWRITE, NULL );
}
