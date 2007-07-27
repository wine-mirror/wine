/*
 * msvcrt.dll heap functions
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
 *
 * Note: Win32 heap operations are MT safe. We only lock the new
 *       handler and non atomic heap operations
 */

#include "msvcrt.h"
#include "mtdll.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* MT */
#define LOCK_HEAP   _mlock( _HEAP_LOCK )
#define UNLOCK_HEAP _munlock( _HEAP_LOCK )


typedef void (*MSVCRT_new_handler_func)(unsigned long size);

static MSVCRT_new_handler_func MSVCRT_new_handler;
static int MSVCRT_new_mode;

/* FIXME - According to documentation it should be 8*1024, at runtime it returns 16 */ 
static unsigned int MSVCRT_amblksiz = 16;
/* FIXME - According to documentation it should be 480 bytes, at runtime default is 0 */
static size_t MSVCRT_sbh_threshold = 0;

/*********************************************************************
 *		??2@YAPAXI@Z (MSVCRT.@)
 */
void* CDECL MSVCRT_operator_new(unsigned long size)
{
  void *retval = HeapAlloc(GetProcessHeap(), 0, size);
  TRACE("(%ld) returning %p\n", size, retval);
  if(retval) return retval;
  LOCK_HEAP;
  if(MSVCRT_new_handler)
    (*MSVCRT_new_handler)(size);
  UNLOCK_HEAP;
  return retval;
}

/*********************************************************************
 *		??3@YAXPAX@Z (MSVCRT.@)
 */
void CDECL MSVCRT_operator_delete(void *mem)
{
  TRACE("(%p)\n", mem);
  HeapFree(GetProcessHeap(), 0, mem);
}


/*********************************************************************
 *		?_query_new_handler@@YAP6AHI@ZXZ (MSVCRT.@)
 */
MSVCRT_new_handler_func CDECL MSVCRT__query_new_handler(void)
{
  return MSVCRT_new_handler;
}


/*********************************************************************
 *		?_query_new_mode@@YAHXZ (MSVCRT.@)
 */
int CDECL MSVCRT__query_new_mode(void)
{
  return MSVCRT_new_mode;
}

/*********************************************************************
 *		?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z (MSVCRT.@)
 */
MSVCRT_new_handler_func CDECL MSVCRT__set_new_handler(MSVCRT_new_handler_func func)
{
  MSVCRT_new_handler_func old_handler;
  LOCK_HEAP;
  old_handler = MSVCRT_new_handler;
  MSVCRT_new_handler = func;
  UNLOCK_HEAP;
  return old_handler;
}

/*********************************************************************
 *		?set_new_handler@@YAP6AXXZP6AXXZ@Z (MSVCRT.@)
 */
MSVCRT_new_handler_func CDECL MSVCRT_set_new_handler(void *func)
{
  TRACE("(%p)\n",func);
  MSVCRT__set_new_handler(NULL);
  return NULL;
}

/*********************************************************************
 *		?_set_new_mode@@YAHH@Z (MSVCRT.@)
 */
int CDECL MSVCRT__set_new_mode(int mode)
{
  int old_mode;
  LOCK_HEAP;
  old_mode = MSVCRT_new_mode;
  MSVCRT_new_mode = mode;
  UNLOCK_HEAP;
  return old_mode;
}

/*********************************************************************
 *		_callnewh (MSVCRT.@)
 */
int CDECL _callnewh(unsigned long size)
{
  if(MSVCRT_new_handler)
    (*MSVCRT_new_handler)(size);
  return 0;
}

/*********************************************************************
 *		_expand (MSVCRT.@)
 */
void* CDECL _expand(void* mem, MSVCRT_size_t size)
{
  return HeapReAlloc(GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY, mem, size);
}

/*********************************************************************
 *		_heapchk (MSVCRT.@)
 */
int CDECL _heapchk(void)
{
  if (!HeapValidate( GetProcessHeap(), 0, NULL))
  {
    msvcrt_set_errno(GetLastError());
    return MSVCRT__HEAPBADNODE;
  }
  return MSVCRT__HEAPOK;
}

/*********************************************************************
 *		_heapmin (MSVCRT.@)
 */
int CDECL _heapmin(void)
{
  if (!HeapCompact( GetProcessHeap(), 0 ))
  {
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
      msvcrt_set_errno(GetLastError());
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_heapwalk (MSVCRT.@)
 */
int CDECL _heapwalk(struct MSVCRT__heapinfo* next)
{
  PROCESS_HEAP_ENTRY phe;

  LOCK_HEAP;
  phe.lpData = next->_pentry;
  phe.cbData = next->_size;
  phe.wFlags = next->_useflag == MSVCRT__USEDENTRY ? PROCESS_HEAP_ENTRY_BUSY : 0;

  if (phe.lpData && phe.wFlags & PROCESS_HEAP_ENTRY_BUSY &&
      !HeapValidate( GetProcessHeap(), 0, phe.lpData ))
  {
    UNLOCK_HEAP;
    msvcrt_set_errno(GetLastError());
    return MSVCRT__HEAPBADNODE;
  }

  do
  {
    if (!HeapWalk( GetProcessHeap(), &phe ))
    {
      UNLOCK_HEAP;
      if (GetLastError() == ERROR_NO_MORE_ITEMS)
         return MSVCRT__HEAPEND;
      msvcrt_set_errno(GetLastError());
      if (!phe.lpData)
        return MSVCRT__HEAPBADBEGIN;
      return MSVCRT__HEAPBADNODE;
    }
  } while (phe.wFlags & (PROCESS_HEAP_REGION|PROCESS_HEAP_UNCOMMITTED_RANGE));

  UNLOCK_HEAP;
  next->_pentry = phe.lpData;
  next->_size = phe.cbData;
  next->_useflag = phe.wFlags & PROCESS_HEAP_ENTRY_BUSY ? MSVCRT__USEDENTRY : MSVCRT__FREEENTRY;
  return MSVCRT__HEAPOK;
}

/*********************************************************************
 *		_heapset (MSVCRT.@)
 */
int CDECL _heapset(unsigned int value)
{
  int retval;
  struct MSVCRT__heapinfo heap;

  memset( &heap, 0, sizeof(heap) );
  LOCK_HEAP;
  while ((retval = _heapwalk(&heap)) == MSVCRT__HEAPOK)
  {
    if (heap._useflag == MSVCRT__FREEENTRY)
      memset(heap._pentry, value, heap._size);
  }
  UNLOCK_HEAP;
  return retval == MSVCRT__HEAPEND? MSVCRT__HEAPOK : retval;
}

/*********************************************************************
 *		_heapadd (MSVCRT.@)
 */
int CDECL _heapadd(void* mem, MSVCRT_size_t size)
{
  TRACE("(%p,%d) unsupported in Win32\n", mem,size);
  *MSVCRT__errno() = MSVCRT_ENOSYS;
  return -1;
}

/*********************************************************************
 *		_msize (MSVCRT.@)
 */
MSVCRT_size_t CDECL _msize(void* mem)
{
  long size = HeapSize(GetProcessHeap(),0,mem);
  if (size == -1)
  {
    WARN(":Probably called with non wine-allocated memory, ret = -1\n");
    /* At least the Win32 crtdll/msvcrt also return -1 in this case */
  }
  return size;
}

/*********************************************************************
 *		calloc (MSVCRT.@)
 */
void* CDECL MSVCRT_calloc(MSVCRT_size_t size, MSVCRT_size_t count)
{
  return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size * count );
}

/*********************************************************************
 *		free (MSVCRT.@)
 */
void CDECL MSVCRT_free(void* ptr)
{
  HeapFree(GetProcessHeap(),0,ptr);
}

/*********************************************************************
 *                  malloc (MSVCRT.@)
 */
void* CDECL MSVCRT_malloc(MSVCRT_size_t size)
{
  void *ret = HeapAlloc(GetProcessHeap(),0,size);
  if (!ret)
    msvcrt_set_errno(MSVCRT_ENOMEM);
  return ret;
}

/*********************************************************************
 *		realloc (MSVCRT.@)
 */
void* CDECL MSVCRT_realloc(void* ptr, MSVCRT_size_t size)
{
  if (!ptr) return MSVCRT_malloc(size);
  if (size) return HeapReAlloc(GetProcessHeap(), 0, ptr, size);
  MSVCRT_free(ptr);
  return NULL;
}

/*********************************************************************
 *		__p__amblksiz (MSVCRT.@)
 */
unsigned int* CDECL __p__amblksiz(void)
{
  return &MSVCRT_amblksiz;
}

/*********************************************************************
 *		_get_sbh_threshold (MSVCRT.@)
 */
size_t CDECL _get_sbh_threshold(void)
{
  return MSVCRT_sbh_threshold;
}

/*********************************************************************
 *		_set_sbh_threshold (MSVCRT.@)
 */
int CDECL _set_sbh_threshold(size_t threshold)
{
  if(threshold > 1016)
     return 0;
  else
     MSVCRT_sbh_threshold = threshold;
  return 1;
}
