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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnt.h"
#include "winreg.h"
#include "winternl.h"
#include "excpt.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(heap);

/* address where we try to map the system heap */
#define SYSTEM_HEAP_BASE  ((void*)0x65430000)
#define SYSTEM_HEAP_SIZE  0x100000   /* Default heap size = 1Mb */

static HANDLE systemHeap;   /* globally shared heap */



/* filter for page-fault exceptions */
/* It is possible for a bogus global pointer to cause a */
/* page zero reference, so I include EXCEPTION_PRIV_INSTRUCTION too. */
static WINE_EXCEPTION_FILTER(page_fault)
{
    switch (GetExceptionCode()) {
        case (EXCEPTION_ACCESS_VIOLATION):
        case (EXCEPTION_PRIV_INSTRUCTION):
           return EXCEPTION_EXECUTE_HANDLER;
        default:
           return EXCEPTION_CONTINUE_SEARCH;
    }
}

/***********************************************************************
 *           HEAP_CreateSystemHeap
 *
 * Create the system heap.
 */
inline static HANDLE HEAP_CreateSystemHeap(void)
{
    int created;
    void *base;
    HANDLE map, event;
    UNICODE_STRING event_name;
    OBJECT_ATTRIBUTES event_attr;

    if (!(map = CreateFileMappingA( INVALID_HANDLE_VALUE, NULL, SEC_COMMIT | PAGE_READWRITE,
                                    0, SYSTEM_HEAP_SIZE, "__SystemHeap" ))) return 0;
    created = (GetLastError() != ERROR_ALREADY_EXISTS);

    if (!(base = MapViewOfFileEx( map, FILE_MAP_ALL_ACCESS, 0, 0, 0, SYSTEM_HEAP_BASE )))
    {
        /* pre-defined address not available */
        ERR( "system heap base address %p not available\n", SYSTEM_HEAP_BASE );
        return 0;
    }

    /* create the system heap event */
    RtlCreateUnicodeStringFromAsciiz( &event_name, "__SystemHeapEvent" );
    event_attr.Length = sizeof(event_attr);
    event_attr.RootDirectory = 0;
    event_attr.ObjectName = &event_name;
    event_attr.Attributes = 0;
    event_attr.SecurityDescriptor = NULL;
    event_attr.SecurityQualityOfService = NULL;
    NtCreateEvent( &event, EVENT_ALL_ACCESS, &event_attr, TRUE, FALSE );

    if (created)  /* newly created heap */
    {
        systemHeap = RtlCreateHeap( HEAP_SHARED, base, SYSTEM_HEAP_SIZE,
                                    SYSTEM_HEAP_SIZE, NULL, NULL );
        NtSetEvent( event, NULL );
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
 *           GetProcessHeaps    (KERNEL32.@)
 */
DWORD WINAPI GetProcessHeaps( DWORD count, HANDLE *heaps )
{
    return RtlGetProcessHeaps( count, heaps );
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
#define GLOBAL_LOCK_MAX   0xFF
#define HANDLE_TO_INTERN(h)  ((PGLOBAL32_INTERN)(((char *)(h))-2))
#define INTERN_TO_HANDLE(i)  ((HGLOBAL) &((i)->Pointer))
#define POINTER_TO_HANDLE(p) (*(((HGLOBAL *)(p))-2))
#define ISHANDLE(h)          (((ULONG_PTR)(h)&2)!=0)
#define ISPOINTER(h)         (((ULONG_PTR)(h)&2)==0)
/* align the storage needed for the HGLOBAL on an 8byte boundary thus
 * GlobalAlloc/GlobalReAlloc'ing with GMEM_MOVEABLE of memory with
 * size = 8*k, where k=1,2,3,... alloc's exactly the given size.
 * The Minolta DiMAGE Image Viewer heavily relies on this, corrupting
 * the output jpeg's > 1 MB if not */
#define HGLOBAL_STORAGE      8  /* sizeof(HGLOBAL)*2 */

typedef struct __GLOBAL32_INTERN
{
   WORD         Magic;
   LPVOID       Pointer WINE_PACKED;
   BYTE         Flags;
   BYTE         LockCount;
} GLOBAL32_INTERN, *PGLOBAL32_INTERN;


/***********************************************************************
 *           GlobalAlloc   (KERNEL32.@)
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

   TRACE("() flags=%04x\n",  flags );

   if((flags & GMEM_MOVEABLE)==0) /* POINTER */
   {
      palloc=HeapAlloc(GetProcessHeap(), hpflags, size);
      return (HGLOBAL) palloc;
   }
   else  /* HANDLE */
   {
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
      return pintern ? INTERN_TO_HANDLE(pintern) : 0;
   }
}


/***********************************************************************
 *           GlobalLock   (KERNEL32.@)
 * RETURNS
 *      Pointer to first byte of block
 *      NULL: Failure
 */
LPVOID WINAPI GlobalLock(
              HGLOBAL hmem /* [in] Handle of global memory object */
)
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
            if (pintern->LockCount < GLOBAL_LOCK_MAX)
                pintern->LockCount++;
            palloc = pintern->Pointer;
        }
        else
        {
            WARN("invalid handle %p\n", hmem);
            palloc = NULL;
            SetLastError(ERROR_INVALID_HANDLE);
        }
    }
    __EXCEPT(page_fault)
    {
        WARN("page fault on %p\n", hmem);
        palloc = NULL;
        SetLastError(ERROR_INVALID_HANDLE);
    }
    __ENDTRY
    RtlUnlockHeap(GetProcessHeap());
    return palloc;
}


/***********************************************************************
 *           GlobalUnlock   (KERNEL32.@)
 * RETURNS
 *      TRUE: Object is still locked
 *      FALSE: Object is unlocked
 */
BOOL WINAPI GlobalUnlock(
              HGLOBAL hmem /* [in] Handle of global memory object */
) {
    PGLOBAL32_INTERN pintern;
    BOOL locked;

    if (ISPOINTER(hmem)) return FALSE;

    RtlLockHeap(GetProcessHeap());
    __TRY
    {
        pintern=HANDLE_TO_INTERN(hmem);
        if(pintern->Magic==MAGIC_GLOBAL_USED)
        {
            if((pintern->LockCount<GLOBAL_LOCK_MAX)&&(pintern->LockCount>0))
                pintern->LockCount--;

            locked = (pintern->LockCount != 0);
            if (!locked) SetLastError(NO_ERROR);
        }
        else
        {
            WARN("invalid handle\n");
            SetLastError(ERROR_INVALID_HANDLE);
            locked=FALSE;
        }
    }
    __EXCEPT(page_fault)
    {
        ERR("page fault occurred ! Caused by bug ?\n");
        SetLastError( ERROR_INVALID_PARAMETER );
        locked=FALSE;
    }
    __ENDTRY
    RtlUnlockHeap(GetProcessHeap());
    return locked;
}


/***********************************************************************
 *           GlobalHandle   (KERNEL32.@)
 * Returns the handle associated with the specified pointer.
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

        /* note that if pmem is a pointer to a a block allocated by        */
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
            if (HeapValidate( GetProcessHeap(), 0, (char *)test - HGLOBAL_STORAGE ) && /* obj(-handle) valid arena? */
                HeapValidate( GetProcessHeap(), 0, maybe_intern ))  /* intern valid arena? */
                break;  /* valid moveable block */
        }
        handle = 0;
        SetLastError( ERROR_INVALID_HANDLE );
    }
    __EXCEPT(page_fault)
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
         hnew=(HGLOBAL)HeapReAlloc(GetProcessHeap(), heap_flags, (LPVOID) hmem, size);
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
               if((palloc = HeapReAlloc(GetProcessHeap(), heap_flags,
                                   (char *) pintern->Pointer-HGLOBAL_STORAGE,
                                   size+HGLOBAL_STORAGE)) == NULL)
                   hnew = 0; /* Block still valid */
               else
                   pintern->Pointer = (char *)palloc+HGLOBAL_STORAGE;
            }
            else
            {
                if((palloc=HeapAlloc(GetProcessHeap(), heap_flags, size+HGLOBAL_STORAGE))
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
            if(pintern->Pointer)
            {
               HeapFree(GetProcessHeap(), 0, (char *) pintern->Pointer-HGLOBAL_STORAGE);
               pintern->Pointer=NULL;
            }
         }
      }
   }
   RtlUnlockHeap(GetProcessHeap());
   return hnew;
}


/***********************************************************************
 *           GlobalFree   (KERNEL32.@)
 * RETURNS
 *      NULL: Success
 *      Handle: Failure
 */
HGLOBAL WINAPI GlobalFree(
                 HGLOBAL hmem /* [in] Handle of global memory object */
) {
    PGLOBAL32_INTERN pintern;
    HGLOBAL hreturned;

    RtlLockHeap(GetProcessHeap());
    __TRY
    {
        hreturned = 0;
        if(ISPOINTER(hmem)) /* POINTER */
        {
            if(!HeapFree(GetProcessHeap(), 0, (LPVOID) hmem)) hmem = 0;
        }
        else  /* HANDLE */
        {
            pintern=HANDLE_TO_INTERN(hmem);

            if(pintern->Magic==MAGIC_GLOBAL_USED)
            {

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
        }
    }
    __EXCEPT(page_fault)
    {
        ERR("page fault occurred ! Caused by bug ?\n");
        SetLastError( ERROR_INVALID_PARAMETER );
        hreturned = hmem;
    }
    __ENDTRY
    RtlUnlockHeap(GetProcessHeap());
    return hreturned;
}


/***********************************************************************
 *           GlobalSize   (KERNEL32.@)
 * RETURNS
 *      Size in bytes of the global memory object
 *      0: Failure
 */
SIZE_T WINAPI GlobalSize(
             HGLOBAL hmem /* [in] Handle of global memory object */
) {
   DWORD                retval;
   PGLOBAL32_INTERN     pintern;

   if (!hmem) return 0;

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
         WARN("invalid handle\n");
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
 * Returns information about the specified global memory object
 *
 * NOTES
 *      Should this return GMEM_INVALID_HANDLE on invalid handle?
 *
 * RETURNS
 *      Value specifying allocation flags and lock count
 *      GMEM_INVALID_HANDLE: Failure
 */
UINT WINAPI GlobalFlags(
              HGLOBAL hmem /* [in] Handle to global memory object */
) {
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
         WARN("Invalid handle: %p\n", hmem);
         retval=0;
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
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
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
 * RETURNS
 *	Value specifying allocation flags and lock count.
 *	LMEM_INVALID_HANDLE: Failure
 */
UINT WINAPI LocalFlags(
              HLOCAL handle /* [in] Handle of memory object */
) {
    return GlobalFlags( (HGLOBAL)handle );
}


/***********************************************************************
 *           LocalFree   (KERNEL32.@)
 * RETURNS
 *	NULL: Success
 *	Handle: Failure
 */
HLOCAL WINAPI LocalFree(
                HLOCAL handle /* [in] Handle of memory object */
) {
    return (HLOCAL)GlobalFree( (HGLOBAL)handle );
}


/***********************************************************************
 *           LocalHandle   (KERNEL32.@)
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 */
HLOCAL WINAPI LocalHandle(
                LPCVOID ptr /* [in] Address of local memory object */
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
 */
LPVOID WINAPI LocalLock(
              HLOCAL handle /* [in] Address of local memory object */
) {
    return GlobalLock( (HGLOBAL)handle );
}


/***********************************************************************
 *           LocalReAlloc   (KERNEL32.@)
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
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
 * RETURNS
 *	Size: Success
 *	0: Failure
 */
SIZE_T WINAPI LocalSize(
              HLOCAL handle /* [in] Handle of memory object */
) {
    return GlobalSize( (HGLOBAL)handle );
}


/***********************************************************************
 *           LocalUnlock   (KERNEL32.@)
 * RETURNS
 *	TRUE: Object is still locked
 *	FALSE: Object is unlocked
 */
BOOL WINAPI LocalUnlock(
              HLOCAL handle /* [in] Handle of memory object */
) {
    return GlobalUnlock( (HGLOBAL)handle );
}


/**********************************************************************
 * 		AllocMappedBuffer	(KERNEL32.38)
 *
 * This is a undocumented KERNEL32 function that
 * SMapLS's a GlobalAlloc'ed buffer.
 *
 * Input:   EDI register: size of buffer to allocate
 * Output:  EDI register: pointer to buffer
 *
 * Note: The buffer is preceded by 8 bytes:
 *        ...
 *       edi+0   buffer
 *       edi-4   SEGPTR to buffer
 *       edi-8   some magic Win95 needs for SUnMapLS
 *               (we use it for the memory handle)
 *
 *       The SEGPTR is used by the caller!
 */
void WINAPI AllocMappedBuffer( CONTEXT86 *context )
{
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


/**********************************************************************
 * 		FreeMappedBuffer	(KERNEL32.39)
 *
 * Free a buffer allocated by AllocMappedBuffer
 *
 * Input: EDI register: pointer to buffer
 */
void WINAPI FreeMappedBuffer( CONTEXT86 *context )
{
    if (context->Edi)
    {
        DWORD *buffer = (DWORD *)context->Edi - 2;

        UnMapLS(buffer[1]);

        GlobalUnlock((HGLOBAL)buffer[0]);
        GlobalFree((HGLOBAL)buffer[0]);
    }
}
