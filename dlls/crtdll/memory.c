/*
 * CRTDLL memory functions
 * 
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 *
 * Implementation Notes:
 * MT Safe.
 * heapwalk from win does not work. This is most likely due to internal
 * differences between wine and win (see memory/heap.c comments). This
 * version works fine, however.
 */

#include "crtdll.h"


DEFAULT_DEBUG_CHANNEL(crtdll);

static new_handler_type new_handler;


/*********************************************************************
 *                  new           (CRTDLL.001)
 *
 * Allocate memory.
 */
LPVOID __cdecl CRTDLL_new(DWORD size)
{
    VOID* result;
    if(!(result = HeapAlloc(GetProcessHeap(),0,size)) && new_handler)
	(*new_handler)();
    return result;
}


/*********************************************************************
 *                  delete       (CRTDLL.002)
 *
 * Free memory created with new.
 */
VOID __cdecl CRTDLL_delete(LPVOID ptr)
{
    HeapFree(GetProcessHeap(),0,ptr);
}


/*********************************************************************
 *                  set_new_handler(CRTDLL.003)
 */
new_handler_type __cdecl CRTDLL_set_new_handler(new_handler_type func)
{
    new_handler_type old_handler = new_handler;
    new_handler = func;
    return old_handler;
}


/*********************************************************************
 *                  _expand        (CRTDLL.088)
 *
 * Increase the size of a block of memory allocated with malloc()
 * or realloc().
 */
LPVOID __cdecl CRTDLL__expand(LPVOID ptr, INT size)
{
    return HeapReAlloc( GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY, ptr, size );
}


/*********************************************************************
 *                  _heapchk        (CRTDLL.130)
 *
 * Check the consistency of the process heap.
 */
INT __cdecl CRTDLL__heapchk(VOID)
{
  if (!HeapValidate( GetProcessHeap(), 0, NULL))
  {
    __CRTDLL__set_errno(GetLastError());
    return _HEAPBADNODE;
  }
  return _HEAPOK;
}


/*********************************************************************
 *                  _heapmin        (CRTDLL.131)
 *
 * Minimise the size of the heap.
 */
INT __cdecl CRTDLL__heapmin(VOID)
{
  if (!HeapCompact( GetProcessHeap(), 0 ))
  {
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
      __CRTDLL__set_errno(GetLastError());
    return -1;
  }
  return 0;
}


/*********************************************************************
 *                  _heapset        (CRTDLL.132)
 *
 * Fill free memory in the heap with a given value.
 */
INT __cdecl CRTDLL__heapset(UINT value)
{
  INT retVal;
  struct _heapinfo heap;

  memset( &heap, 0, sizeof(heap) );

  while ((retVal = CRTDLL__heapwalk(&heap)) == _HEAPOK)
  {
    if (heap._useflag == _FREEENTRY)
      memset(heap._pentry, value, heap._size);
  }
  return retVal == _HEAPEND? _HEAPOK : retVal;
}


/*********************************************************************
 *                  _heapwalk        (CRTDLL.133)
 *
 * Walk the heap block by block.
 */
INT __cdecl CRTDLL__heapwalk(struct _heapinfo *next)
{
  PROCESS_HEAP_ENTRY phe;

  phe.lpData = next->_pentry;
  phe.cbData = next->_size;
  phe.wFlags = next->_useflag == _USEDENTRY ? PROCESS_HEAP_ENTRY_BUSY : 0;

  if (phe.lpData && phe.wFlags & PROCESS_HEAP_ENTRY_BUSY && 
      !HeapValidate( GetProcessHeap(), 0, phe.lpData ))
  {
    __CRTDLL__set_errno(GetLastError());
    return _HEAPBADNODE;
  }

  do
  {
    if (!HeapWalk( GetProcessHeap(), &phe ))
    {
      if (GetLastError() == ERROR_NO_MORE_ITEMS)
        return _HEAPEND;
      __CRTDLL__set_errno(GetLastError());
      if (!phe.lpData)
        return _HEAPBADBEGIN;
      return _HEAPBADNODE;
    }
  } while (phe.wFlags & (PROCESS_HEAP_REGION|PROCESS_HEAP_UNCOMMITTED_RANGE));

  next->_pentry = phe.lpData;
  next->_size = phe.cbData;
  next->_useflag = phe.wFlags & PROCESS_HEAP_ENTRY_BUSY ? _USEDENTRY : _FREEENTRY;
  return _HEAPOK;
}


/*********************************************************************
 *                  _msize     (CRTDLL.234)
 *
 * Return the actual used size of an allocated block of memory.
 *
 */
LONG __cdecl CRTDLL__msize(LPVOID mem)
{
  LONG size = HeapSize(GetProcessHeap(),0,mem);
  if (size == -1)
  {
    WARN(":Probably called with non wine-allocated memory, ret = -1\n");
    /* At least the win98/nt crtdlls also return -1 in this case */
  }
  return size;
}


/*********************************************************************
 *                  calloc        (CRTDLL.350)
 *
 * Allocate memory from the heap and initialise it to zero.
 */
LPVOID __cdecl CRTDLL_calloc(DWORD size, DWORD count)
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size * count );
}


/*********************************************************************
 *                  free          (CRTDLL.375)
 *
 * Free a block of memory allocated with malloc()
 */
VOID __cdecl CRTDLL_free(LPVOID ptr)
{
    HeapFree(GetProcessHeap(),0,ptr);
}


/*********************************************************************
 *                  malloc        (CRTDLL.424)
 * 
 * Alocate memory from the heap.
 */
LPVOID __cdecl CRTDLL_malloc(DWORD size)
{
  LPVOID ret = HeapAlloc(GetProcessHeap(),0,size);
  if (!ret)
    __CRTDLL__set_errno(GetLastError());
  return ret;
}


/*********************************************************************
 *                  realloc        (CRTDLL.444)
 *
 * Resize a block of memory allocated with malloc() or realloc().
 */
LPVOID __cdecl CRTDLL_realloc( VOID *ptr, DWORD size )
{
    return HeapReAlloc( GetProcessHeap(), 0, ptr, size );
}
