/*
 * msvcrt.dll heap functions
 *
 * Copyright 2000 Jon Griffiths
 *
 * Note: Win32 heap operations are MT safe. We only lock the new
 *       handler and non atomic heap operations
 */

#include "msvcrt.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);

/* MT */
extern CRITICAL_SECTION MSVCRT_heap_cs;
#define LOCK_HEAP      EnterCriticalSection(&MSVCRT_heap_cs)
#define UNLOCK_HEAP    LeaveCriticalSection(&MSVCRT_heap_cs)

/* heap function constants */
#define MSVCRT_HEAPEMPTY    -1
#define MSVCRT_HEAPOK       -2
#define MSVCRT_HEAPBADBEGIN -3
#define MSVCRT_HEAPBADNODE  -4
#define MSVCRT_HEAPEND      -5
#define MSVCRT_HEAPBADPTR   -6
#define MSVCRT_FREEENTRY    0
#define MSVCRT_USEDENTRY    1

typedef struct MSVCRT_heapinfo
{
  int * _pentry;
  size_t _size;
  int    _useflag;
} MSVCRT_HEAPINFO;

typedef void (*MSVCRT_new_handler_func)(void);

static MSVCRT_new_handler_func MSVCRT_new_handler;
static int MSVCRT_new_mode;

/*********************************************************************
 *		operator_new (MSVCRT.@)
 */
void* MSVCRT_operator_new(unsigned long size)
{
  void *retval = HeapAlloc(GetProcessHeap(), 0, size);
  TRACE("(%ld) returning %p\n", size, retval);
  LOCK_HEAP;
  if(retval && MSVCRT_new_handler)
    (*MSVCRT_new_handler)();
  UNLOCK_HEAP;
  return retval;
}

/*********************************************************************
 *		operator_delete (MSVCRT.@)
 */
void MSVCRT_operator_delete(void *mem)
{
  TRACE("(%p)\n", mem);
  HeapFree(GetProcessHeap(), 0, mem);
}


/*********************************************************************
 *		?_query_new_handler@@YAP6AHI@ZXZ (MSVCRT.@)
 */
MSVCRT_new_handler_func MSVCRT__query_new_handler(void)
{
  return MSVCRT_new_handler;
}


/*********************************************************************
 *		?_query_new_mode@@YAHXZ (MSVCRT.@)
 */
int MSVCRT__query_new_mode(void)
{
  return MSVCRT_new_mode;
}

/*********************************************************************
 *		?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z (MSVCRT.@)
 */
MSVCRT_new_handler_func MSVCRT__set_new_handler(MSVCRT_new_handler_func func)
{
  MSVCRT_new_handler_func old_handler;
  LOCK_HEAP;
  old_handler = MSVCRT_new_handler;
  MSVCRT_new_handler = func;
  UNLOCK_HEAP;
  return old_handler;
}

/*********************************************************************
 *		?_set_new_mode@@YAHH@Z (MSVCRT.@)
 */
int MSVCRT__set_new_mode(int mode)
{
  int old_mode;
  LOCK_HEAP;
  old_mode = MSVCRT_new_mode;
  MSVCRT_new_mode = mode;
  UNLOCK_HEAP;
  return old_mode;
}

/*********************************************************************
 *		_expand (MSVCRT.@)
 */
void* _expand(void* mem, unsigned int size)
{
  return HeapReAlloc(GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY, mem, size);
}

/*********************************************************************
 *		_heapchk (MSVCRT.@)
 */
int _heapchk(void)
{
  if (!HeapValidate( GetProcessHeap(), 0, NULL))
  {
    MSVCRT__set_errno(GetLastError());
    return MSVCRT_HEAPBADNODE;
  }
  return MSVCRT_HEAPOK;
}

/*********************************************************************
 *		_heapmin (MSVCRT.@)
 */
int _heapmin(void)
{
  if (!HeapCompact( GetProcessHeap(), 0 ))
  {
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
      MSVCRT__set_errno(GetLastError());
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_heapwalk (MSVCRT.@)
 */
int _heapwalk(MSVCRT_HEAPINFO* next)
{
  PROCESS_HEAP_ENTRY phe;

  LOCK_HEAP;
  phe.lpData = next->_pentry;
  phe.cbData = next->_size;
  phe.wFlags = next->_useflag == MSVCRT_USEDENTRY ? PROCESS_HEAP_ENTRY_BUSY : 0;

  if (phe.lpData && phe.wFlags & PROCESS_HEAP_ENTRY_BUSY &&
      !HeapValidate( GetProcessHeap(), 0, phe.lpData ))
  {
    UNLOCK_HEAP;
    MSVCRT__set_errno(GetLastError());
    return MSVCRT_HEAPBADNODE;
  }

  do
  {
    if (!HeapWalk( GetProcessHeap(), &phe ))
    {
      UNLOCK_HEAP;
      if (GetLastError() == ERROR_NO_MORE_ITEMS)
         return MSVCRT_HEAPEND;
      MSVCRT__set_errno(GetLastError());
      if (!phe.lpData)
        return MSVCRT_HEAPBADBEGIN;
      return MSVCRT_HEAPBADNODE;
    }
  } while (phe.wFlags & (PROCESS_HEAP_REGION|PROCESS_HEAP_UNCOMMITTED_RANGE));

  UNLOCK_HEAP;
  next->_pentry = phe.lpData;
  next->_size = phe.cbData;
  next->_useflag = phe.wFlags & PROCESS_HEAP_ENTRY_BUSY ? MSVCRT_USEDENTRY : MSVCRT_FREEENTRY;
  return MSVCRT_HEAPOK;
}

/*********************************************************************
 *		_heapset (MSVCRT.@)
 */
int _heapset(unsigned int value)
{
  int retval;
  MSVCRT_HEAPINFO heap;

  memset( &heap, 0, sizeof(MSVCRT_HEAPINFO) );
  LOCK_HEAP;
  while ((retval = _heapwalk(&heap)) == MSVCRT_HEAPOK)
  {
    if (heap._useflag == MSVCRT_FREEENTRY)
      memset(heap._pentry, value, heap._size);
  }
  UNLOCK_HEAP;
  return retval == MSVCRT_HEAPEND? MSVCRT_HEAPOK : retval;
}

/*********************************************************************
 *		_msize (MSVCRT.@)
 */
long _msize(void* mem)
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
void* MSVCRT_calloc(unsigned int size, unsigned int count)
{
  return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size * count );
}

/*********************************************************************
 *		free (MSVCRT.@)
 */
void MSVCRT_free(void* ptr)
{
  HeapFree(GetProcessHeap(),0,ptr);
}

/*********************************************************************
 *                  malloc (MSVCRT.@)
 */
void* MSVCRT_malloc(unsigned int size)
{
  void *ret = HeapAlloc(GetProcessHeap(),0,size);
  if (!ret)
    MSVCRT__set_errno(GetLastError());
  return ret;
}

/*********************************************************************
 *		realloc (MSVCRT.@)
 */
void* MSVCRT_realloc(void* ptr, unsigned int size)
{
  return HeapReAlloc(GetProcessHeap(), 0, ptr, size);
}
