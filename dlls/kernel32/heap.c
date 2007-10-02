/*
 * Win32 heap functions
 *
 * Copyright 1995, 1996 Alexandre Julliard
 * Copyright 1996 Huw Davies
 * Copyright 1998 Ulrich Weigand
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

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef sun
/* FIXME:  Unfortunately swapctl can't be used with largefile.... */
# undef _FILE_OFFSET_BITS
# define _FILE_OFFSET_BITS 32
# ifdef HAVE_SYS_RESOURCE_H
#  include <sys/resource.h>
# endif
# ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>
# endif
# include <sys/swap.h>
#endif


#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(heap);

/* address where we try to map the system heap */
#define SYSTEM_HEAP_BASE  ((void*)0x80000000)
#define SYSTEM_HEAP_SIZE  0x1000000   /* Default heap size = 16Mb */

static HANDLE systemHeap;   /* globally shared heap */


/***********************************************************************
 *           HEAP_CreateSystemHeap
 *
 * Create the system heap.
 */
static inline HANDLE HEAP_CreateSystemHeap(void)
{
    int created;
    void *base;
    HANDLE map, event;

    /* create the system heap event first */
    event = CreateEventA( NULL, TRUE, FALSE, "__wine_system_heap_event" );

    if (!(map = CreateFileMappingA( INVALID_HANDLE_VALUE, NULL, SEC_COMMIT | PAGE_READWRITE,
                                    0, SYSTEM_HEAP_SIZE, "__wine_system_heap" ))) return 0;
    created = (GetLastError() != ERROR_ALREADY_EXISTS);

    if (!(base = MapViewOfFileEx( map, FILE_MAP_ALL_ACCESS, 0, 0, 0, SYSTEM_HEAP_BASE )))
    {
        /* pre-defined address not available */
        ERR( "system heap base address %p not available\n", SYSTEM_HEAP_BASE );
        return 0;
    }

    if (created)  /* newly created heap */
    {
        systemHeap = RtlCreateHeap( HEAP_SHARED, base, SYSTEM_HEAP_SIZE,
                                    SYSTEM_HEAP_SIZE, NULL, NULL );
        SetEvent( event );
    }
    else
    {
        /* wait for the heap to be initialized */
        WaitForSingleObject( event, INFINITE );
        systemHeap = (HANDLE)base;
    }
    CloseHandle( map );
    return systemHeap;
}


/***********************************************************************
 *           HeapCreate   (KERNEL32.@)
 *
 * Create a heap object.
 *
 * RETURNS
 *	Handle of heap: Success
 *	NULL: Failure
 */
HANDLE WINAPI HeapCreate(
                DWORD flags,       /* [in] Heap allocation flag */
                SIZE_T initialSize, /* [in] Initial heap size */
                SIZE_T maxSize      /* [in] Maximum heap size */
) {
    HANDLE ret;

    if ( flags & HEAP_SHARED )
    {
        if (!systemHeap) HEAP_CreateSystemHeap();
        else WARN( "Shared Heap requested, returning system heap.\n" );
        ret = systemHeap;
    }
    else
    {
        ret = RtlCreateHeap( flags, NULL, maxSize, initialSize, NULL, NULL );
        if (!ret) SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    }
    return ret;
}


/***********************************************************************
 *           HeapDestroy   (KERNEL32.@)
 *
 * Destroy a heap object.
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI HeapDestroy( HANDLE heap /* [in] Handle of heap */ )
{
    if (heap == systemHeap)
    {
        WARN( "attempt to destroy system heap, returning TRUE!\n" );
        return TRUE;
    }
    if (!RtlDestroyHeap( heap )) return TRUE;
    SetLastError( ERROR_INVALID_HANDLE );
    return FALSE;
}


/***********************************************************************
 *           HeapCompact   (KERNEL32.@)
 */
SIZE_T WINAPI HeapCompact( HANDLE heap, DWORD flags )
{
    return RtlCompactHeap( heap, flags );
}


/***********************************************************************
 *           HeapValidate   (KERNEL32.@)
 * Validates a specified heap.
 *
 * NOTES
 *	Flags is ignored.
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI HeapValidate(
              HANDLE heap, /* [in] Handle to the heap */
              DWORD flags,   /* [in] Bit flags that control access during operation */
              LPCVOID block  /* [in] Optional pointer to memory block to validate */
) {
    return RtlValidateHeap( heap, flags, block );
}


/***********************************************************************
 *           HeapWalk   (KERNEL32.@)
 * Enumerates the memory blocks in a specified heap.
 *
 * TODO
 *   - handling of PROCESS_HEAP_ENTRY_MOVEABLE and
 *     PROCESS_HEAP_ENTRY_DDESHARE (needs heap.c support)
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI HeapWalk(
              HANDLE heap,               /* [in]  Handle to heap to enumerate */
              LPPROCESS_HEAP_ENTRY entry /* [out] Pointer to structure of enumeration info */
) {
    NTSTATUS ret = RtlWalkHeap( heap, entry );
    if (ret) SetLastError( RtlNtStatusToDosError(ret) );
    return !ret;
}


/***********************************************************************
 *           HeapLock   (KERNEL32.@)
 * Attempts to acquire the critical section object for a specified heap.
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI HeapLock(
              HANDLE heap /* [in] Handle of heap to lock for exclusive access */
) {
    return RtlLockHeap( heap );
}


/***********************************************************************
 *           HeapUnlock   (KERNEL32.@)
 * Releases ownership of the critical section object.
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI HeapUnlock(
              HANDLE heap /* [in] Handle to the heap to unlock */
) {
    return RtlUnlockHeap( heap );
}


/***********************************************************************
 *           GetProcessHeap    (KERNEL32.@)
 */
HANDLE WINAPI GetProcessHeap(void)
{
    return NtCurrentTeb()->Peb->ProcessHeap;
}


/***********************************************************************
 *           GetProcessHeaps    (KERNEL32.@)
 */
DWORD WINAPI GetProcessHeaps( DWORD count, HANDLE *heaps )
{
    return RtlGetProcessHeaps( count, heaps );
}


/* These are needed so that we can call the functions from inside kernel itself */

/***********************************************************************
 *           HeapAlloc    (KERNEL32.@)
 */
LPVOID WINAPI HeapAlloc( HANDLE heap, DWORD flags, SIZE_T size )
{
    return RtlAllocateHeap( heap, flags, size );
}

BOOL WINAPI HeapFree( HANDLE heap, DWORD flags, LPVOID ptr )
{
    return RtlFreeHeap( heap, flags, ptr );
}

LPVOID WINAPI HeapReAlloc( HANDLE heap, DWORD flags, LPVOID ptr, SIZE_T size )
{
    return RtlReAllocateHeap( heap, flags, ptr, size );
}

SIZE_T WINAPI HeapSize( HANDLE heap, DWORD flags, LPCVOID ptr )
{
    return RtlSizeHeap( heap, flags, ptr );
}

BOOL WINAPI HeapSetInformation( HANDLE heap, HEAP_INFORMATION_CLASS infoclass, PVOID info, SIZE_T size)
{
    FIXME("%p %d %p %ld\n", heap, infoclass, info, size );
    return TRUE;
}

/*
 * Win32 Global heap functions (GlobalXXX).
 * These functions included in Win32 for compatibility with 16 bit Windows
 * Especially the moveable blocks and handles are oldish.
 * But the ability to directly allocate memory with GPTR and LPTR is widely
 * used.
 *
 * The handle stuff looks horrible, but it's implemented almost like Win95
 * does it.
 *
 */

#define MAGIC_GLOBAL_USED 0x5342
#define HANDLE_TO_INTERN(h)  ((PGLOBAL32_INTERN)(((char *)(h))-2))
#define INTERN_TO_HANDLE(i)  ((HGLOBAL) &((i)->Pointer))
#define POINTER_TO_HANDLE(p) (*(((const HGLOBAL *)(p))-2))
#define ISHANDLE(h)          (((ULONG_PTR)(h)&2)!=0)
#define ISPOINTER(h)         (((ULONG_PTR)(h)&2)==0)
/* align the storage needed for the HGLOBAL on an 8byte boundary thus
 * GlobalAlloc/GlobalReAlloc'ing with GMEM_MOVEABLE of memory with
 * size = 8*k, where k=1,2,3,... alloc's exactly the given size.
 * The Minolta DiMAGE Image Viewer heavily relies on this, corrupting
 * the output jpeg's > 1 MB if not */
#define HGLOBAL_STORAGE      8  /* sizeof(HGLOBAL)*2 */

#include "pshpack1.h"

typedef struct __GLOBAL32_INTERN
{
   WORD         Magic;
   LPVOID       Pointer;
   BYTE         Flags;
   BYTE         LockCount;
} GLOBAL32_INTERN, *PGLOBAL32_INTERN;

#include "poppack.h"

/***********************************************************************
 *           GlobalAlloc   (KERNEL32.@)
 *
 * Allocate a global memory object.
 *
 * RETURNS
 *      Handle: Success
 *      NULL: Failure
 */
HGLOBAL WINAPI GlobalAlloc(
                 UINT flags, /* [in] Object allocation attributes */
                 SIZE_T size /* [in] Number of bytes to allocate */
) {
   PGLOBAL32_INTERN     pintern;
   DWORD                hpflags;
   LPVOID               palloc;

   if(flags&GMEM_ZEROINIT)
      hpflags=HEAP_ZERO_MEMORY;
   else
      hpflags=0;

   if((flags & GMEM_MOVEABLE)==0) /* POINTER */
   {
      palloc=HeapAlloc(GetProcessHeap(), hpflags, size);
      TRACE( "(flags=%04x) returning %p\n",  flags, palloc );
      return (HGLOBAL) palloc;
   }
   else  /* HANDLE */
   {
      if (size > INT_MAX-HGLOBAL_STORAGE)
      {
          SetLastError(ERROR_OUTOFMEMORY);
          return 0;
      }

      RtlLockHeap(GetProcessHeap());

      pintern = HeapAlloc(GetProcessHeap(), 0, sizeof(GLOBAL32_INTERN));
      if (pintern)
      {
          pintern->Magic = MAGIC_GLOBAL_USED;
          pintern->Flags = flags >> 8;
          pintern->LockCount = 0;

          if (size)
          {
              palloc = HeapAlloc(GetProcessHeap(), hpflags, size+HGLOBAL_STORAGE);
              if (!palloc)
              {
                  HeapFree(GetProcessHeap(), 0, pintern);
                  pintern = NULL;
              }
              else
              {
                  *(HGLOBAL *)palloc = INTERN_TO_HANDLE(pintern);
                  pintern->Pointer = (char *)palloc + HGLOBAL_STORAGE;
              }
          }
          else
              pintern->Pointer = NULL;
      }

      RtlUnlockHeap(GetProcessHeap());
      if (!pintern) return 0;
      TRACE( "(flags=%04x) returning handle %p pointer %p\n",
             flags, INTERN_TO_HANDLE(pintern), pintern->Pointer );
      return INTERN_TO_HANDLE(pintern);
   }
}


/***********************************************************************
 *           GlobalLock   (KERNEL32.@)
 *
 * Lock a global memory object and return a pointer to first byte of the memory
 *
 * PARAMS
 *  hmem [I] Handle of the global memory object
 *
 * RETURNS
 *  Success: Pointer to first byte of the memory block
 *  Failure: NULL
 *
 * NOTES
 *   When the handle is invalid, last error is set to ERROR_INVALID_HANDLE
 *
 */
LPVOID WINAPI GlobalLock(HGLOBAL hmem)
{
    PGLOBAL32_INTERN pintern;
    LPVOID           palloc;

    if (ISPOINTER(hmem))
        return IsBadReadPtr(hmem, 1) ? NULL : hmem;

    RtlLockHeap(GetProcessHeap());
    __TRY
    {
        pintern = HANDLE_TO_INTERN(hmem);
        if (pintern->Magic == MAGIC_GLOBAL_USED)
        {
            palloc = pintern->Pointer;
            if (!pintern->Pointer)
                SetLastError(ERROR_DISCARDED);
            else if (pintern->LockCount < GMEM_LOCKCOUNT)
                pintern->LockCount++;
        }
        else
        {
            WARN("invalid handle %p (Magic: 0x%04x)\n", hmem, pintern->Magic);
            palloc = NULL;
            SetLastError(ERROR_INVALID_HANDLE);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        WARN("(%p): Page fault occurred ! Caused by bug ?\n", hmem);
        palloc = NULL;
        SetLastError(ERROR_INVALID_HANDLE);
    }
    __ENDTRY
    RtlUnlockHeap(GetProcessHeap());
    return palloc;
}


/***********************************************************************
 *           GlobalUnlock   (KERNEL32.@)
 *
 * Unlock a global memory object.
 *
 * PARAMS
 *  hmem [I] Handle of the global memory object
 *
 * RETURNS
 *  Success: Object is still locked
 *  Failure: FALSE (The Object is unlocked)
 *
 * NOTES
 *   When the handle is invalid, last error is set to ERROR_INVALID_HANDLE
 *
 */
BOOL WINAPI GlobalUnlock(HGLOBAL hmem)
{
    PGLOBAL32_INTERN pintern;
    BOOL locked;

    if (ISPOINTER(hmem)) return TRUE;

    RtlLockHeap(GetProcessHeap());
    __TRY
    {
        pintern=HANDLE_TO_INTERN(hmem);
        if(pintern->Magic==MAGIC_GLOBAL_USED)
        {
            if(pintern->LockCount)
            {
                pintern->LockCount--;
                locked = (pintern->LockCount != 0);
                if (!locked) SetLastError(NO_ERROR);
            }
            else
            {
                WARN("%p not locked\n", hmem);
                SetLastError(ERROR_NOT_LOCKED);
                locked = FALSE;
            }
        }
        else
        {
            WARN("invalid handle %p (Magic: 0x%04x)\n", hmem, pintern->Magic);
            SetLastError(ERROR_INVALID_HANDLE);
            locked=FALSE;
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        WARN("(%p): Page fault occurred ! Caused by bug ?\n", hmem);
        SetLastError( ERROR_INVALID_PARAMETER );
        locked=FALSE;
    }
    __ENDTRY
    RtlUnlockHeap(GetProcessHeap());
    return locked;
}


/***********************************************************************
 *           GlobalHandle   (KERNEL32.@)
 *
 * Get the handle associated with the pointer to a global memory block.
 *
 * RETURNS
 *      Handle: Success
 *      NULL: Failure
 */
HGLOBAL WINAPI GlobalHandle(
                 LPCVOID pmem /* [in] Pointer to global memory block */
) {
    HGLOBAL handle;
    PGLOBAL32_INTERN  maybe_intern;
    LPCVOID test;

    if (!pmem)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    RtlLockHeap(GetProcessHeap());
    __TRY
    {
        handle = 0;

        /* note that if pmem is a pointer to a block allocated by        */
        /* GlobalAlloc with GMEM_MOVEABLE then magic test in HeapValidate  */
        /* will fail.                                                      */
        if (ISPOINTER(pmem)) {
            if (HeapValidate( GetProcessHeap(), 0, pmem )) {
                handle = (HGLOBAL)pmem;  /* valid fixed block */
                break;
            }
            handle = POINTER_TO_HANDLE(pmem);
        } else
            handle = (HGLOBAL)pmem;

        /* Now test handle either passed in or retrieved from pointer */
        maybe_intern = HANDLE_TO_INTERN( handle );
        if (maybe_intern->Magic == MAGIC_GLOBAL_USED) {
            test = maybe_intern->Pointer;
            if (HeapValidate( GetProcessHeap(), 0, (const char *)test - HGLOBAL_STORAGE ) && /* obj(-handle) valid arena? */
                HeapValidate( GetProcessHeap(), 0, maybe_intern ))  /* intern valid arena? */
                break;  /* valid moveable block */
        }
        handle = 0;
        SetLastError( ERROR_INVALID_HANDLE );
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_HANDLE );
        handle = 0;
    }
    __ENDTRY
    RtlUnlockHeap(GetProcessHeap());

    return handle;
}


/***********************************************************************
 *           GlobalReAlloc   (KERNEL32.@)
 *
 * Change the size or attributes of a global memory object.
 *
 * RETURNS
 *      Handle: Success
 *      NULL: Failure
 */
HGLOBAL WINAPI GlobalReAlloc(
                 HGLOBAL hmem, /* [in] Handle of global memory object */
                 SIZE_T size,  /* [in] New size of block */
                 UINT flags    /* [in] How to reallocate object */
) {
   LPVOID               palloc;
   HGLOBAL            hnew;
   PGLOBAL32_INTERN     pintern;
   DWORD heap_flags = (flags & GMEM_ZEROINIT) ? HEAP_ZERO_MEMORY : 0;

   hnew = 0;
   RtlLockHeap(GetProcessHeap());
   if(flags & GMEM_MODIFY) /* modify flags */
   {
      if( ISPOINTER(hmem) && (flags & GMEM_MOVEABLE))
      {
         /* make a fixed block moveable
          * actually only NT is able to do this. But it's soo simple
          */
         if (hmem == 0)
         {
             WARN("GlobalReAlloc with null handle!\n");
             SetLastError( ERROR_NOACCESS );
             hnew = 0;
         }
         else
         {
             size = HeapSize(GetProcessHeap(), 0, (LPVOID)hmem);
             hnew = GlobalAlloc(flags, size);
             palloc = GlobalLock(hnew);
             memcpy(palloc, (LPVOID)hmem, size);
             GlobalUnlock(hnew);
             GlobalFree(hmem);
         }
      }
      else if( ISPOINTER(hmem) &&(flags & GMEM_DISCARDABLE))
      {
         /* change the flags to make our block "discardable" */
         pintern=HANDLE_TO_INTERN(hmem);
         pintern->Flags = pintern->Flags | (GMEM_DISCARDABLE >> 8);
         hnew=hmem;
      }
      else
      {
         SetLastError(ERROR_INVALID_PARAMETER);
         hnew = 0;
      }
   }
   else
   {
      if(ISPOINTER(hmem))
      {
         /* reallocate fixed memory */
         hnew=HeapReAlloc(GetProcessHeap(), heap_flags, hmem, size);
      }
      else
      {
         /* reallocate a moveable block */
         pintern=HANDLE_TO_INTERN(hmem);

#if 0
/* Apparently Windows doesn't care whether the handle is locked at this point */
/* See also the same comment in GlobalFree() */
         if(pintern->LockCount>1) {
            ERR("handle 0x%08lx is still locked, cannot realloc!\n",(DWORD)hmem);
            SetLastError(ERROR_INVALID_HANDLE);
         } else
#endif
         if(size!=0)
         {
            hnew=hmem;
            if(pintern->Pointer)
            {
               if(size > INT_MAX-HGLOBAL_STORAGE)
               {
                   SetLastError(ERROR_OUTOFMEMORY);
                   hnew = 0;
               }
               else if((palloc = HeapReAlloc(GetProcessHeap(), heap_flags,
                                   (char *) pintern->Pointer-HGLOBAL_STORAGE,
                                   size+HGLOBAL_STORAGE)) == NULL)
                   hnew = 0; /* Block still valid */
               else
                   pintern->Pointer = (char *)palloc+HGLOBAL_STORAGE;
            }
            else
            {
                if(size > INT_MAX-HGLOBAL_STORAGE)
                {
                    SetLastError(ERROR_OUTOFMEMORY);
                    hnew = 0;
                }
                else if((palloc=HeapAlloc(GetProcessHeap(), heap_flags, size+HGLOBAL_STORAGE))
                   == NULL)
                    hnew = 0;
                else
                {
                    *(HGLOBAL *)palloc = hmem;
                    pintern->Pointer = (char *)palloc + HGLOBAL_STORAGE;
                }
            }
         }
         else
         {
            if (pintern->LockCount == 0)
            {
                if(pintern->Pointer)
                {
                    HeapFree(GetProcessHeap(), 0, (char *) pintern->Pointer-HGLOBAL_STORAGE);
                    pintern->Pointer = NULL;
                }
                hnew = hmem;
            }
            else
                WARN("not freeing memory associated with locked handle\n");
         }
      }
   }
   RtlUnlockHeap(GetProcessHeap());
   return hnew;
}


/***********************************************************************
 *           GlobalFree   (KERNEL32.@)
 *
 * Free a global memory object.
 *
 * PARAMS
 *  hmem [I] Handle of the global memory object
 *
 * RETURNS
 *  Success: NULL
 *  Failure: The provided handle
 *
 * NOTES
 *   When the handle is invalid, last error is set to ERROR_INVALID_HANDLE
 *
 */
HGLOBAL WINAPI GlobalFree(HGLOBAL hmem)
{
    PGLOBAL32_INTERN pintern;
    HGLOBAL hreturned;

    RtlLockHeap(GetProcessHeap());
    __TRY
    {
        hreturned = 0;
        if(ISPOINTER(hmem)) /* POINTER */
        {
            if(!HeapFree(GetProcessHeap(), 0, (LPVOID) hmem))
            {
                SetLastError(ERROR_INVALID_HANDLE);
                hreturned = hmem;
            }
        }
        else  /* HANDLE */
        {
            pintern=HANDLE_TO_INTERN(hmem);

            if(pintern->Magic==MAGIC_GLOBAL_USED)
            {
                pintern->Magic = 0xdead;

                /* WIN98 does not make this test. That is you can free a */
                /* block you have not unlocked. Go figure!!              */
                /* if(pintern->LockCount!=0)  */
                /*    SetLastError(ERROR_INVALID_HANDLE);  */

                if(pintern->Pointer)
                    if(!HeapFree(GetProcessHeap(), 0, (char *)(pintern->Pointer)-HGLOBAL_STORAGE))
                        hreturned=hmem;
                if(!HeapFree(GetProcessHeap(), 0, pintern))
                    hreturned=hmem;
            }
            else
            {
                WARN("invalid handle %p (Magic: 0x%04x)\n", hmem, pintern->Magic);
                SetLastError(ERROR_INVALID_HANDLE);
                hreturned = hmem;
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        ERR("(%p): Page fault occurred ! Caused by bug ?\n", hmem);
        SetLastError( ERROR_INVALID_PARAMETER );
        hreturned = hmem;
    }
    __ENDTRY
    RtlUnlockHeap(GetProcessHeap());
    return hreturned;
}


/***********************************************************************
 *           GlobalSize   (KERNEL32.@)
 *
 * Get the size of a global memory object.
 *
 * PARAMS
 *  hmem [I] Handle of the global memory object
 *
 * RETURNS
 *  Failure: 0
 *  Success: Size in Bytes of the global memory object
 *
 * NOTES
 *   When the handle is invalid, last error is set to ERROR_INVALID_HANDLE
 *
 */
SIZE_T WINAPI GlobalSize(HGLOBAL hmem)
{
   DWORD                retval;
   PGLOBAL32_INTERN     pintern;

   if (!((ULONG_PTR)hmem >> 16))
   {
       SetLastError(ERROR_INVALID_HANDLE);
       return 0;
   }

   if(ISPOINTER(hmem))
   {
      retval=HeapSize(GetProcessHeap(), 0,  (LPVOID) hmem);
   }
   else
   {
      RtlLockHeap(GetProcessHeap());
      pintern=HANDLE_TO_INTERN(hmem);

      if(pintern->Magic==MAGIC_GLOBAL_USED)
      {
         if (!pintern->Pointer) /* handle case of GlobalAlloc( ??,0) */
             retval = 0;
         else
         {
             retval = HeapSize(GetProcessHeap(), 0,
                         (char *)(pintern->Pointer) - HGLOBAL_STORAGE );
             if (retval != (DWORD)-1) retval -= HGLOBAL_STORAGE;
         }
      }
      else
      {
         WARN("invalid handle %p (Magic: 0x%04x)\n", hmem, pintern->Magic);
         SetLastError(ERROR_INVALID_HANDLE);
         retval=0;
      }
      RtlUnlockHeap(GetProcessHeap());
   }
   /* HeapSize returns 0xffffffff on failure */
   if (retval == 0xffffffff) retval = 0;
   return retval;
}


/***********************************************************************
 *           GlobalWire   (KERNEL32.@)
 */
LPVOID WINAPI GlobalWire(HGLOBAL hmem)
{
   return GlobalLock( hmem );
}


/***********************************************************************
 *           GlobalUnWire   (KERNEL32.@)
 */
BOOL WINAPI GlobalUnWire(HGLOBAL hmem)
{
   return GlobalUnlock( hmem);
}


/***********************************************************************
 *           GlobalFix   (KERNEL32.@)
 */
VOID WINAPI GlobalFix(HGLOBAL hmem)
{
    GlobalLock( hmem );
}


/***********************************************************************
 *           GlobalUnfix   (KERNEL32.@)
 */
VOID WINAPI GlobalUnfix(HGLOBAL hmem)
{
   GlobalUnlock( hmem);
}


/***********************************************************************
 *           GlobalFlags   (KERNEL32.@)
 *
 * Get information about a global memory object.
 *
 * PARAMS
 *  hmem [I] Handle of the global memory object 
 *
 * RETURNS
 *  Failure: GMEM_INVALID_HANDLE, when the provided handle is invalid 
 *  Success: Value specifying allocation flags and lock count
 *
 */
UINT WINAPI GlobalFlags(HGLOBAL hmem)
{
   DWORD                retval;
   PGLOBAL32_INTERN     pintern;

   if(ISPOINTER(hmem))
   {
      retval=0;
   }
   else
   {
      RtlLockHeap(GetProcessHeap());
      pintern=HANDLE_TO_INTERN(hmem);
      if(pintern->Magic==MAGIC_GLOBAL_USED)
      {
         retval=pintern->LockCount + (pintern->Flags<<8);
         if(pintern->Pointer==0)
            retval|= GMEM_DISCARDED;
      }
      else
      {
         WARN("invalid handle %p (Magic: 0x%04x)\n", hmem, pintern->Magic);
         SetLastError(ERROR_INVALID_HANDLE);
         retval = GMEM_INVALID_HANDLE;
      }
      RtlUnlockHeap(GetProcessHeap());
   }
   return retval;
}


/***********************************************************************
 *           GlobalCompact   (KERNEL32.@)
 */
SIZE_T WINAPI GlobalCompact( DWORD minfree )
{
    return 0;  /* GlobalCompact does nothing in Win32 */
}


/***********************************************************************
 *           LocalAlloc   (KERNEL32.@)
 *
 * Allocate a local memory object.
 *
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 *
 * NOTES
 *  Windows memory management does not provide a separate local heap
 *  and global heap.
 */
HLOCAL WINAPI LocalAlloc(
                UINT flags, /* [in] Allocation attributes */
                SIZE_T size /* [in] Number of bytes to allocate */
) {
    return (HLOCAL)GlobalAlloc( flags, size );
}


/***********************************************************************
 *           LocalCompact   (KERNEL32.@)
 */
SIZE_T WINAPI LocalCompact( UINT minfree )
{
    return 0;  /* LocalCompact does nothing in Win32 */
}


/***********************************************************************
 *           LocalFlags   (KERNEL32.@)
 *
 * Get information about a local memory object.
 *
 * RETURNS
 *	Value specifying allocation flags and lock count.
 *	LMEM_INVALID_HANDLE: Failure
 *
 * NOTES
 *  Windows memory management does not provide a separate local heap
 *  and global heap.
 */
UINT WINAPI LocalFlags(
              HLOCAL handle /* [in] Handle of memory object */
) {
    return GlobalFlags( (HGLOBAL)handle );
}


/***********************************************************************
 *           LocalFree   (KERNEL32.@)
 *
 * Free a local memory object.
 *
 * RETURNS
 *	NULL: Success
 *	Handle: Failure
 *
 * NOTES
 *  Windows memory management does not provide a separate local heap
 *  and global heap.
 */
HLOCAL WINAPI LocalFree(
                HLOCAL handle /* [in] Handle of memory object */
) {
    return (HLOCAL)GlobalFree( (HGLOBAL)handle );
}


/***********************************************************************
 *           LocalHandle   (KERNEL32.@)
 *
 * Get the handle associated with the pointer to a local memory block.
 *
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 *
 * NOTES
 *  Windows memory management does not provide a separate local heap
 *  and global heap.
 */
HLOCAL WINAPI LocalHandle(
                LPCVOID ptr /* [in] Address of local memory block */
) {
    return (HLOCAL)GlobalHandle( ptr );
}


/***********************************************************************
 *           LocalLock   (KERNEL32.@)
 * Locks a local memory object and returns pointer to the first byte
 * of the memory block.
 *
 * RETURNS
 *	Pointer: Success
 *	NULL: Failure
 *
 * NOTES
 *  Windows memory management does not provide a separate local heap
 *  and global heap.
 */
LPVOID WINAPI LocalLock(
              HLOCAL handle /* [in] Address of local memory object */
) {
    return GlobalLock( (HGLOBAL)handle );
}


/***********************************************************************
 *           LocalReAlloc   (KERNEL32.@)
 *
 * Change the size or attributes of a local memory object.
 *
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 *
 * NOTES
 *  Windows memory management does not provide a separate local heap
 *  and global heap.
 */
HLOCAL WINAPI LocalReAlloc(
                HLOCAL handle, /* [in] Handle of memory object */
                SIZE_T size,   /* [in] New size of block */
                UINT flags     /* [in] How to reallocate object */
) {
    return (HLOCAL)GlobalReAlloc( (HGLOBAL)handle, size, flags );
}


/***********************************************************************
 *           LocalShrink   (KERNEL32.@)
 */
SIZE_T WINAPI LocalShrink( HGLOBAL handle, UINT newsize )
{
    return 0;  /* LocalShrink does nothing in Win32 */
}


/***********************************************************************
 *           LocalSize   (KERNEL32.@)
 *
 * Get the size of a local memory object.
 *
 * RETURNS
 *	Size: Success
 *	0: Failure
 *
 * NOTES
 *  Windows memory management does not provide a separate local heap
 *  and global heap.
 */
SIZE_T WINAPI LocalSize(
              HLOCAL handle /* [in] Handle of memory object */
) {
    return GlobalSize( (HGLOBAL)handle );
}


/***********************************************************************
 *           LocalUnlock   (KERNEL32.@)
 *
 * Unlock a local memory object.
 *
 * RETURNS
 *	TRUE: Object is still locked
 *	FALSE: Object is unlocked
 *
 * NOTES
 *  Windows memory management does not provide a separate local heap
 *  and global heap.
 */
BOOL WINAPI LocalUnlock(
              HLOCAL handle /* [in] Handle of memory object */
) {
    return GlobalUnlock( (HGLOBAL)handle );
}


/**********************************************************************
 * 		AllocMappedBuffer	(KERNEL32.38)
 *
 * This is an undocumented KERNEL32 function that
 * SMapLS's a GlobalAlloc'ed buffer.
 *
 * RETURNS
 *       EDI register: pointer to buffer
 *
 * NOTES
 *       The buffer is preceded by 8 bytes:
 *        ...
 *       edi+0   buffer
 *       edi-4   SEGPTR to buffer
 *       edi-8   some magic Win95 needs for SUnMapLS
 *               (we use it for the memory handle)
 *
 *       The SEGPTR is used by the caller!
 */
void WINAPI __regs_AllocMappedBuffer(
              CONTEXT86 *context /* [in] EDI register: size of buffer to allocate */
) {
    HGLOBAL handle = GlobalAlloc(0, context->Edi + 8);
    DWORD *buffer = (DWORD *)GlobalLock(handle);
    DWORD ptr = 0;

    if (buffer)
        if (!(ptr = MapLS(buffer + 2)))
        {
            GlobalUnlock(handle);
            GlobalFree(handle);
        }

    if (!ptr)
        context->Eax = context->Edi = 0;
    else
    {
        buffer[0] = (DWORD)handle;
        buffer[1] = ptr;

        context->Eax = (DWORD) ptr;
        context->Edi = (DWORD)(buffer + 2);
    }
}
#ifdef DEFINE_REGS_ENTRYPOINT
DEFINE_REGS_ENTRYPOINT( AllocMappedBuffer, 0, 0 )
#endif

/**********************************************************************
 * 		FreeMappedBuffer	(KERNEL32.39)
 *
 * Free a buffer allocated by AllocMappedBuffer
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI __regs_FreeMappedBuffer(
              CONTEXT86 *context /* [in] EDI register: pointer to buffer */
) {
    if (context->Edi)
    {
        DWORD *buffer = (DWORD *)context->Edi - 2;

        UnMapLS(buffer[1]);

        GlobalUnlock((HGLOBAL)buffer[0]);
        GlobalFree((HGLOBAL)buffer[0]);
    }
}
#ifdef DEFINE_REGS_ENTRYPOINT
DEFINE_REGS_ENTRYPOINT( FreeMappedBuffer, 0, 0 )
#endif

/***********************************************************************
 *           GlobalMemoryStatusEx   (KERNEL32.@)
 * A version of GlobalMemoryStatus that can deal with memory over 4GB
 *
 * RETURNS
 *	TRUE
 */
BOOL WINAPI GlobalMemoryStatusEx( LPMEMORYSTATUSEX lpmemex )
{
    static MEMORYSTATUSEX	cached_memstatus;
    static int cache_lastchecked = 0;
    SYSTEM_INFO si;
#ifdef linux
    FILE *f;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__)
    unsigned long val;
    int mib[2];
    size_t size_sys;
#elif defined(__APPLE__)
    unsigned int val;
    int mib[2];
    size_t size_sys;
#elif defined(sun)
    unsigned long pagesize,maxpages,freepages,swapspace,swapfree;
    struct anoninfo swapinf;
    int rval;
#endif

    if (time(NULL)==cache_lastchecked) {
	memcpy(lpmemex,&cached_memstatus,sizeof(*lpmemex));
	return TRUE;
    }
    cache_lastchecked = time(NULL);

    lpmemex->dwLength         = sizeof(*lpmemex);
    lpmemex->dwMemoryLoad     = 0;
    lpmemex->ullTotalPhys     = 16*1024*1024;
    lpmemex->ullAvailPhys     = 16*1024*1024;
    lpmemex->ullTotalPageFile = 16*1024*1024;
    lpmemex->ullAvailPageFile = 16*1024*1024;

#ifdef linux
    f = fopen( "/proc/meminfo", "r" );
    if (f)
    {
        char buffer[256];
        unsigned long total, used, free, shared, buffers, cached;

        lpmemex->ullTotalPhys = lpmemex->ullAvailPhys = 0;
        lpmemex->ullTotalPageFile = lpmemex->ullAvailPageFile = 0;
        while (fgets( buffer, sizeof(buffer), f ))
        {
	    /* old style /proc/meminfo ... */
            if (sscanf( buffer, "Mem: %lu %lu %lu %lu %lu %lu",
                        &total, &used, &free, &shared, &buffers, &cached ))
            {
                lpmemex->ullTotalPhys += total;
                lpmemex->ullAvailPhys += free + buffers + cached;
            }
            if (sscanf( buffer, "Swap: %lu %lu %lu", &total, &used, &free ))
            {
                lpmemex->ullTotalPageFile += total;
                lpmemex->ullAvailPageFile += free;
            }

            /* new style /proc/meminfo ... */
            if (sscanf(buffer, "MemTotal: %lu", &total))
                lpmemex->ullTotalPhys = total*1024;
            if (sscanf(buffer, "MemFree: %lu", &free))
                lpmemex->ullAvailPhys = free*1024;
            if (sscanf(buffer, "SwapTotal: %lu", &total))
                lpmemex->ullTotalPageFile = total*1024;
            if (sscanf(buffer, "SwapFree: %lu", &free))
                lpmemex->ullAvailPageFile = free*1024;
            if (sscanf(buffer, "Buffers: %lu", &buffers))
                lpmemex->ullAvailPhys += buffers*1024;
            if (sscanf(buffer, "Cached: %lu", &cached))
                lpmemex->ullAvailPhys += cached*1024;
        }
        fclose( f );
    }
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__APPLE__)
    mib[0] = CTL_HW;
    mib[1] = HW_PHYSMEM;
    size_sys = sizeof(val);
    sysctl(mib, 2, &val, &size_sys, NULL, 0);
    if (val) lpmemex->ullTotalPhys = val;
    mib[1] = HW_USERMEM;
    size_sys = sizeof(val);
    sysctl(mib, 2, &val, &size_sys, NULL, 0);
    if (!val) val = lpmemex->ullTotalPhys;
    lpmemex->ullAvailPhys = val;
    lpmemex->ullTotalPageFile = val;
    lpmemex->ullAvailPageFile = val;
#elif defined ( sun )
    pagesize=sysconf(_SC_PAGESIZE);
    maxpages=sysconf(_SC_PHYS_PAGES);
    freepages=sysconf(_SC_AVPHYS_PAGES);
    rval=swapctl(SC_AINFO, &swapinf);
    if(rval >-1)
    {
        swapspace=swapinf.ani_max*pagesize;
        swapfree=swapinf.ani_free*pagesize;
    }else
    {

        WARN("Swap size cannot be determined , assuming equal to physical memory\n");
        swapspace=maxpages*pagesize;
        swapfree=maxpages*pagesize;
    }
    lpmemex->ullTotalPhys=pagesize*maxpages;
    lpmemex->ullAvailPhys = pagesize*freepages;
    lpmemex->ullTotalPageFile = swapspace;
    lpmemex->ullAvailPageFile = swapfree;
#endif

    if (lpmemex->ullTotalPhys)
    {
        lpmemex->dwMemoryLoad = (lpmemex->ullTotalPhys-lpmemex->ullAvailPhys)
                                  / (lpmemex->ullTotalPhys / 100);
    }

    /* Win98 returns only the swapsize in ullTotalPageFile/ullAvailPageFile,
       WinXP returns the size of physical memory + swapsize;
       mimic the behavior of XP.
       Note: Project2k refuses to start if it sees less than 1Mb of free swap.
    */
    lpmemex->ullTotalPageFile += lpmemex->ullTotalPhys;
    lpmemex->ullAvailPageFile += lpmemex->ullAvailPhys;

    /* Titan Quest refuses to run if TotalPageFile <= ullTotalPhys */
    if(lpmemex->ullTotalPageFile == lpmemex->ullTotalPhys)
    {
        lpmemex->ullTotalPhys -= 1;
        lpmemex->ullAvailPhys -= 1;
    }

    /* FIXME: should do something for other systems */
    GetSystemInfo(&si);
    lpmemex->ullTotalVirtual  = (char*)si.lpMaximumApplicationAddress-(char*)si.lpMinimumApplicationAddress;
    /* FIXME: we should track down all the already allocated VM pages and substract them, for now arbitrarily remove 64KB so that it matches NT */
    lpmemex->ullAvailVirtual  = lpmemex->ullTotalVirtual-64*1024;

    /* MSDN says about AvailExtendedVirtual: Size of unreserved and uncommitted
       memory in the extended portion of the virtual address space of the calling
       process, in bytes.
       However, I don't know what this means, so set it to zero :(
    */
    lpmemex->ullAvailExtendedVirtual = 0;

    memcpy(&cached_memstatus,lpmemex,sizeof(*lpmemex));

    TRACE("<-- LPMEMORYSTATUSEX: dwLength %d, dwMemoryLoad %d, ullTotalPhys %s, ullAvailPhys %s,"
          " ullTotalPageFile %s, ullAvailPageFile %s, ullTotalVirtual %s, ullAvailVirtual %s\n",
          lpmemex->dwLength, lpmemex->dwMemoryLoad, wine_dbgstr_longlong(lpmemex->ullTotalPhys),
          wine_dbgstr_longlong(lpmemex->ullAvailPhys), wine_dbgstr_longlong(lpmemex->ullTotalPageFile),
          wine_dbgstr_longlong(lpmemex->ullAvailPageFile), wine_dbgstr_longlong(lpmemex->ullTotalVirtual),
          wine_dbgstr_longlong(lpmemex->ullAvailVirtual) );

    return TRUE;
}

/***********************************************************************
 *           GlobalMemoryStatus   (KERNEL32.@)
 * Provides information about the status of the memory, so apps can tell
 * roughly how much they are able to allocate
 *
 * RETURNS
 *	None
 */
VOID WINAPI GlobalMemoryStatus( LPMEMORYSTATUS lpBuffer )
{
    MEMORYSTATUSEX memstatus;
    OSVERSIONINFOW osver;

    /* Because GlobalMemoryStatus is identical to GlobalMemoryStatusEX save
       for one extra field in the struct, and the lack of a bug, we simply
       call GlobalMemoryStatusEx and copy the values across. */
    GlobalMemoryStatusEx(&memstatus);

    lpBuffer->dwLength = sizeof(*lpBuffer);
    lpBuffer->dwMemoryLoad = memstatus.dwMemoryLoad;

    /* Windows 2000 and later report -1 when values are greater than 4 Gb.
     * NT reports values modulo 4 Gb.
     * Values between 2 Gb and 4 Gb are rounded down to 2 Gb.
     */

    osver.dwOSVersionInfoSize = sizeof(osver);
    GetVersionExW(&osver);

    if ( osver.dwMajorVersion >= 5 )
    {
        lpBuffer->dwTotalPhys = (memstatus.ullTotalPhys > MAXDWORD) ? MAXDWORD :
                                (memstatus.ullTotalPhys > MAXLONG) ? MAXLONG : memstatus.ullTotalPhys;
        lpBuffer->dwAvailPhys = (memstatus.ullAvailPhys > MAXDWORD) ? MAXDWORD :
                                (memstatus.ullAvailPhys > MAXLONG) ? MAXLONG : memstatus.ullAvailPhys; 
        lpBuffer->dwTotalPageFile = (memstatus.ullTotalPageFile > MAXDWORD) ? MAXDWORD :
                                    (memstatus.ullTotalPageFile > MAXLONG) ? MAXLONG : memstatus.ullTotalPageFile;
        lpBuffer->dwAvailPageFile = (memstatus.ullAvailPageFile > MAXDWORD) ? MAXDWORD :
                                    (memstatus.ullAvailPageFile > MAXLONG) ? MAXLONG : memstatus.ullAvailPageFile;
        lpBuffer->dwTotalVirtual = (memstatus.ullTotalVirtual > MAXDWORD) ? MAXDWORD :
                                   (memstatus.ullTotalVirtual > MAXLONG)  ? MAXLONG : memstatus.ullTotalVirtual;
        lpBuffer->dwAvailVirtual = (memstatus.ullAvailVirtual > MAXDWORD) ? MAXDWORD :
                                   (memstatus.ullAvailVirtual > MAXLONG) ? MAXLONG : memstatus.ullAvailVirtual;
    }
    else	/* duplicate NT bug */
    {
        lpBuffer->dwTotalPhys = (memstatus.ullTotalPhys > MAXDWORD) ? memstatus.ullTotalPhys :
                                (memstatus.ullTotalPhys > MAXLONG) ? MAXLONG : memstatus.ullTotalPhys;
        lpBuffer->dwAvailPhys = (memstatus.ullAvailPhys > MAXDWORD) ? memstatus.ullAvailPhys :
                                (memstatus.ullAvailPhys > MAXLONG) ? MAXLONG : memstatus.ullAvailPhys;
        lpBuffer->dwTotalPageFile = (memstatus.ullTotalPageFile > MAXDWORD) ? memstatus.ullTotalPageFile : 
                                    (memstatus.ullTotalPageFile > MAXLONG) ? MAXLONG : memstatus.ullTotalPageFile;
        lpBuffer->dwAvailPageFile = (memstatus.ullAvailPageFile > MAXDWORD) ? memstatus.ullAvailPageFile : 
                                    (memstatus.ullAvailPageFile > MAXLONG) ? MAXLONG : memstatus.ullAvailPageFile;
        lpBuffer->dwTotalVirtual = (memstatus.ullTotalVirtual > MAXDWORD) ? memstatus.ullTotalVirtual : 
                                   (memstatus.ullTotalVirtual > MAXLONG)  ? MAXLONG : memstatus.ullTotalVirtual;
        lpBuffer->dwAvailVirtual = (memstatus.ullAvailVirtual > MAXDWORD) ? memstatus.ullAvailVirtual :
                                   (memstatus.ullAvailVirtual > MAXLONG) ? MAXLONG : memstatus.ullAvailVirtual;
    }

    /* work around for broken photoshop 4 installer */
    if ( lpBuffer->dwAvailPhys +  lpBuffer->dwAvailPageFile >= 2U*1024*1024*1024)
         lpBuffer->dwAvailPageFile = 2U*1024*1024*1024 -  lpBuffer->dwAvailPhys - 1;
}
