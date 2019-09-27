/*
 * Win32 memory management functions
 *
 * Copyright 1997 Alexandre Julliard
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "winerror.h"

#include "kernelbase.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(heap);


/***********************************************************************
 * Virtual memory functions
 ***********************************************************************/


/***********************************************************************
 *             FlushViewOfFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FlushViewOfFile( const void *base, SIZE_T size )
{
    NTSTATUS status = NtFlushVirtualMemory( GetCurrentProcess(), &base, &size, 0 );

    if (status == STATUS_NOT_MAPPED_DATA) status = STATUS_SUCCESS;
    return set_ntstatus( status );
}


/***********************************************************************
 *          GetSystemFileCacheSize   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetSystemFileCacheSize( SIZE_T *mincache, SIZE_T *maxcache, DWORD *flags )
{
    FIXME( "stub: %p %p %p\n", mincache, maxcache, flags );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *             GetWriteWatch   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetWriteWatch( DWORD flags, void *base, SIZE_T size, void **addresses,
                                             ULONG_PTR *count, ULONG *granularity )
{
    if (!set_ntstatus( NtGetWriteWatch( GetCurrentProcess(), flags, base, size,
                                        addresses, count, granularity )))
        return ~0u;
    return 0;
}


/***********************************************************************
 *             MapViewOfFile   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH MapViewOfFile( HANDLE mapping, DWORD access, DWORD offset_high,
                                               DWORD offset_low, SIZE_T count )
{
    return MapViewOfFileEx( mapping, access, offset_high, offset_low, count, NULL );
}


/***********************************************************************
 *             MapViewOfFileEx   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH MapViewOfFileEx( HANDLE handle, DWORD access, DWORD offset_high,
                                                 DWORD offset_low, SIZE_T count, LPVOID addr )
{
    NTSTATUS status;
    LARGE_INTEGER offset;
    ULONG protect;
    BOOL exec;

    offset.u.LowPart  = offset_low;
    offset.u.HighPart = offset_high;

    exec = access & FILE_MAP_EXECUTE;
    access &= ~FILE_MAP_EXECUTE;

    if (access == FILE_MAP_COPY)
        protect = exec ? PAGE_EXECUTE_WRITECOPY : PAGE_WRITECOPY;
    else if (access & FILE_MAP_WRITE)
        protect = exec ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
    else if (access & FILE_MAP_READ)
        protect = exec ? PAGE_EXECUTE_READ : PAGE_READONLY;
    else protect = PAGE_NOACCESS;

    if ((status = NtMapViewOfSection( handle, GetCurrentProcess(), &addr, 0, 0, &offset,
                                      &count, ViewShare, 0, protect )) < 0)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        addr = NULL;
    }
    return addr;
}


/***********************************************************************
 *	       ReadProcessMemory   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadProcessMemory( HANDLE process, const void *addr, void *buffer,
                                                 SIZE_T size, SIZE_T *bytes_read )
{
    return set_ntstatus( NtReadVirtualMemory( process, addr, buffer, size, bytes_read ));
}


/***********************************************************************
 *             ResetWriteWatch   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH ResetWriteWatch( void *base, SIZE_T size )
{
    if (!set_ntstatus( NtResetWriteWatch( GetCurrentProcess(), base, size )))
        return ~0u;
    return 0;
}


/***********************************************************************
 *          SetSystemFileCacheSize   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetSystemFileCacheSize( SIZE_T mincache, SIZE_T maxcache, DWORD flags )
{
    FIXME( "stub: %ld %ld %d\n", mincache, maxcache, flags );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *             UnmapViewOfFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH UnmapViewOfFile( const void *addr )
{
    if (GetVersion() & 0x80000000)
    {
        MEMORY_BASIC_INFORMATION info;
        if (!VirtualQuery( addr, &info, sizeof(info) ) || info.AllocationBase != addr)
        {
            SetLastError( ERROR_INVALID_ADDRESS );
            return FALSE;
        }
    }
    return set_ntstatus( NtUnmapViewOfSection( GetCurrentProcess(), (void *)addr ));
}


/***********************************************************************
 *             VirtualAlloc   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH VirtualAlloc( void *addr, SIZE_T size, DWORD type, DWORD protect )
{
    return VirtualAllocEx( GetCurrentProcess(), addr, size, type, protect );
}


/***********************************************************************
 *             VirtualAllocEx   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH VirtualAllocEx( HANDLE process, void *addr, SIZE_T size,
                                                DWORD type, DWORD protect )
{
    LPVOID ret = addr;

    if (!set_ntstatus( NtAllocateVirtualMemory( process, &ret, 0, &size, type, protect ))) return NULL;
    return ret;
}


/***********************************************************************
 *             VirtualFree   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH VirtualFree( void *addr, SIZE_T size, DWORD type )
{
    return VirtualFreeEx( GetCurrentProcess(), addr, size, type );
}


/***********************************************************************
 *             VirtualFreeEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH VirtualFreeEx( HANDLE process, void *addr, SIZE_T size, DWORD type )
{
    return set_ntstatus( NtFreeVirtualMemory( process, &addr, &size, type ));
}


/***********************************************************************
 *             VirtualLock   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH  VirtualLock( void *addr, SIZE_T size )
{
    return set_ntstatus( NtLockVirtualMemory( GetCurrentProcess(), &addr, &size, 1 ));
}


/***********************************************************************
 *             VirtualProtect   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH VirtualProtect( void *addr, SIZE_T size, DWORD new_prot, DWORD *old_prot )
{
    return VirtualProtectEx( GetCurrentProcess(), addr, size, new_prot, old_prot );
}


/***********************************************************************
 *             VirtualProtectEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH VirtualProtectEx( HANDLE process, void *addr, SIZE_T size,
                                                DWORD new_prot, DWORD *old_prot )
{
    DWORD prot;

    /* Win9x allows passing NULL as old_prot while this fails on NT */
    if (!old_prot && (GetVersion() & 0x80000000)) old_prot = &prot;
    return set_ntstatus( NtProtectVirtualMemory( process, &addr, &size, new_prot, old_prot ));
}


/***********************************************************************
 *             VirtualQuery   (kernelbase.@)
 */
SIZE_T WINAPI DECLSPEC_HOTPATCH VirtualQuery( LPCVOID addr, PMEMORY_BASIC_INFORMATION info, SIZE_T len )
{
    return VirtualQueryEx( GetCurrentProcess(), addr, info, len );
}


/***********************************************************************
 *             VirtualQueryEx   (kernelbase.@)
 */
SIZE_T WINAPI DECLSPEC_HOTPATCH VirtualQueryEx( HANDLE process, LPCVOID addr,
                                                PMEMORY_BASIC_INFORMATION info, SIZE_T len )
{
    SIZE_T ret;

    if (!set_ntstatus( NtQueryVirtualMemory( process, addr, MemoryBasicInformation, info, len, &ret )))
        return 0;
    return ret;
}


/***********************************************************************
 *             VirtualUnlock   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH VirtualUnlock( void *addr, SIZE_T size )
{
    return set_ntstatus( NtUnlockVirtualMemory( GetCurrentProcess(), &addr, &size, 1 ));
}


/***********************************************************************
 *             WriteProcessMemory    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteProcessMemory( HANDLE process, void *addr, const void *buffer,
                                                  SIZE_T size, SIZE_T *bytes_written )
{
    return set_ntstatus( NtWriteVirtualMemory( process, addr, buffer, size, bytes_written ));
}


/***********************************************************************
 * Heap functions
 ***********************************************************************/


/***********************************************************************
 *           HeapCompact   (kernelbase.@)
 */
SIZE_T WINAPI DECLSPEC_HOTPATCH HeapCompact( HANDLE heap, DWORD flags )
{
    return RtlCompactHeap( heap, flags );
}


/***********************************************************************
 *           HeapCreate   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH HeapCreate( DWORD flags, SIZE_T init_size, SIZE_T max_size )
{
    HANDLE ret = RtlCreateHeap( flags, NULL, max_size, init_size, NULL, NULL );
    if (!ret) SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    return ret;
}


/***********************************************************************
 *           HeapDestroy   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH HeapDestroy( HANDLE heap )
{
    if (!RtlDestroyHeap( heap )) return TRUE;
    SetLastError( ERROR_INVALID_HANDLE );
    return FALSE;
}


/***********************************************************************
 *           HeapLock   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH HeapLock( HANDLE heap )
{
    return RtlLockHeap( heap );
}


/***********************************************************************
 *           HeapQueryInformation   (kernelbase.@)
 */
BOOL WINAPI HeapQueryInformation( HANDLE heap, HEAP_INFORMATION_CLASS info_class,
                                  PVOID info, SIZE_T size, PSIZE_T size_out )
{
    return set_ntstatus( RtlQueryHeapInformation( heap, info_class, info, size, size_out ));
}


/***********************************************************************
 *           HeapSetInformation   (kernelbase.@)
 */
BOOL WINAPI HeapSetInformation( HANDLE heap, HEAP_INFORMATION_CLASS infoclass, PVOID info, SIZE_T size )
{
    return set_ntstatus( RtlSetHeapInformation( heap, infoclass, info, size ));
}


/***********************************************************************
 *           HeapUnlock   (kernelbase.@)
 */
BOOL WINAPI HeapUnlock( HANDLE heap )
{
    return RtlUnlockHeap( heap );
}


/***********************************************************************
 *           HeapValidate   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH HeapValidate( HANDLE heap, DWORD flags, LPCVOID ptr )
{
    return RtlValidateHeap( heap, flags, ptr );
}


/***********************************************************************
 *           HeapWalk   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH HeapWalk( HANDLE heap, PROCESS_HEAP_ENTRY *entry )
{
    return set_ntstatus( RtlWalkHeap( heap, entry ));
}


/***********************************************************************
 * Global/local heap functions
 ***********************************************************************/

#include "pshpack1.h"

struct local_header
{
   WORD  magic;
   void *ptr;
   BYTE flags;
   BYTE lock;
};

#include "poppack.h"

#define MAGIC_LOCAL_USED    0x5342
/* align the storage needed for the HLOCAL on an 8-byte boundary thus
 * LocalAlloc/LocalReAlloc'ing with LMEM_MOVEABLE of memory with
 * size = 8*k, where k=1,2,3,... allocs exactly the given size.
 * The Minolta DiMAGE Image Viewer heavily relies on this, corrupting
 * the output jpeg's > 1 MB if not */
#define HLOCAL_STORAGE      (sizeof(HLOCAL) * 2)

static inline struct local_header *get_header( HLOCAL hmem )
{
    return (struct local_header *)((char *)hmem - 2);
}

static inline HLOCAL get_handle( struct local_header *header )
{
    return &header->ptr;
}

static inline BOOL is_pointer( HLOCAL hmem )
{
    return !((ULONG_PTR)hmem & 2);
}

/***********************************************************************
 *           GlobalAlloc   (kernelbase.@)
 */
HGLOBAL WINAPI DECLSPEC_HOTPATCH GlobalAlloc( UINT flags, SIZE_T size )
{
    /* mask out obsolete flags */
    flags &= ~(GMEM_NOCOMPACT | GMEM_NOT_BANKED | GMEM_NOTIFY);

    /* LocalAlloc allows a 0-size fixed block, but GlobalAlloc doesn't */
    if (!(flags & GMEM_MOVEABLE) && !size) size = 1;

    return LocalAlloc( flags, size );
}


/***********************************************************************
 *           GlobalFree   (kernelbase.@)
 */
HGLOBAL WINAPI DECLSPEC_HOTPATCH GlobalFree( HLOCAL hmem )
{
    return LocalFree( hmem );
}


/***********************************************************************
 *           LocalAlloc   (kernelbase.@)
 */
HLOCAL WINAPI DECLSPEC_HOTPATCH LocalAlloc( UINT flags, SIZE_T size )
{
    struct local_header *header;
    DWORD heap_flags = 0;
    void *ptr;

    if (flags & LMEM_ZEROINIT) heap_flags = HEAP_ZERO_MEMORY;

    if (!(flags & LMEM_MOVEABLE)) /* pointer */
    {
        ptr = HeapAlloc( GetProcessHeap(), heap_flags, size );
        TRACE( "(flags=%04x) returning %p\n",  flags, ptr );
        return ptr;
    }

    if (size > INT_MAX - HLOCAL_STORAGE)
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }
    if (!(header = HeapAlloc( GetProcessHeap(), 0, sizeof(*header) ))) return 0;

    header->magic = MAGIC_LOCAL_USED;
    header->flags = flags >> 8;
    header->lock  = 0;

    if (size)
    {
        if (!(ptr = HeapAlloc(GetProcessHeap(), heap_flags, size + HLOCAL_STORAGE )))
        {
            HeapFree( GetProcessHeap(), 0, header );
            return 0;
        }
        *(HLOCAL *)ptr = get_handle( header );
        header->ptr = (char *)ptr + HLOCAL_STORAGE;
    }
    else header->ptr = NULL;

    TRACE( "(flags=%04x) returning handle %p pointer %p\n",
           flags, get_handle( header ), header->ptr );
    return get_handle( header );
}


/***********************************************************************
 *           LocalFree   (kernelbase.@)
 */
HLOCAL WINAPI DECLSPEC_HOTPATCH LocalFree( HLOCAL hmem )
{
    struct local_header *header;
    HLOCAL ret;

    RtlLockHeap( GetProcessHeap() );
    __TRY
    {
        ret = 0;
        if (is_pointer(hmem)) /* POINTER */
        {
            if (!HeapFree( GetProcessHeap(), HEAP_NO_SERIALIZE, hmem ))
            {
                SetLastError( ERROR_INVALID_HANDLE );
                ret = hmem;
            }
        }
        else  /* HANDLE */
        {
            header = get_header( hmem );
            if (header->magic == MAGIC_LOCAL_USED)
            {
                header->magic = 0xdead;
                if (header->ptr)
                {
                    if (!HeapFree( GetProcessHeap(), HEAP_NO_SERIALIZE,
                                   (char *)header->ptr - HLOCAL_STORAGE ))
                        ret = hmem;
                }
                if (!HeapFree( GetProcessHeap(), HEAP_NO_SERIALIZE, header )) ret = hmem;
            }
            else
            {
                WARN( "invalid handle %p (magic: 0x%04x)\n", hmem, header->magic );
                SetLastError( ERROR_INVALID_HANDLE );
                ret = hmem;
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        WARN( "invalid handle %p\n", hmem );
        SetLastError( ERROR_INVALID_HANDLE );
        ret = hmem;
    }
    __ENDTRY
    RtlUnlockHeap( GetProcessHeap() );
    return ret;
}


/***********************************************************************
 *           LocalLock   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH LocalLock( HLOCAL hmem )
{
    void *ret = NULL;

    if (is_pointer( hmem )) return IsBadReadPtr( hmem, 1 ) ? NULL : hmem;

    RtlLockHeap( GetProcessHeap() );
    __TRY
    {
        struct local_header *header = get_header( hmem );
        if (header->magic == MAGIC_LOCAL_USED)
        {
            ret = header->ptr;
            if (!header->ptr) SetLastError( ERROR_DISCARDED );
            else if (header->lock < LMEM_LOCKCOUNT) header->lock++;
        }
        else
        {
            WARN( "invalid handle %p (magic: 0x%04x)\n", hmem, header->magic );
            SetLastError( ERROR_INVALID_HANDLE );
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        WARN("(%p): Page fault occurred ! Caused by bug ?\n", hmem);
        SetLastError( ERROR_INVALID_HANDLE );
    }
    __ENDTRY
    RtlUnlockHeap( GetProcessHeap() );
    return ret;
}


/***********************************************************************
 *           LocalReAlloc   (kernelbase.@)
 */
HLOCAL WINAPI DECLSPEC_HOTPATCH LocalReAlloc( HLOCAL hmem, SIZE_T size, UINT flags )
{
   struct local_header *header;
   void *ptr;
   HLOCAL ret = 0;
   DWORD heap_flags = (flags & LMEM_ZEROINIT) ? HEAP_ZERO_MEMORY : 0;

   RtlLockHeap( GetProcessHeap() );
   if (flags & LMEM_MODIFY) /* modify flags */
   {
      if (is_pointer( hmem ) && (flags & LMEM_MOVEABLE))
      {
         /* make a fixed block moveable
          * actually only NT is able to do this. But it's soo simple
          */
         if (hmem == 0)
         {
             WARN( "null handle\n");
             SetLastError( ERROR_NOACCESS );
         }
         else
         {
             size = RtlSizeHeap( GetProcessHeap(), 0, hmem );
             ret = LocalAlloc( flags, size );
             ptr = LocalLock( ret );
             memcpy( ptr, hmem, size );
             LocalUnlock( ret );
             LocalFree( hmem );
         }
      }
      else if (!is_pointer( hmem ) && (flags & LMEM_DISCARDABLE))
      {
         /* change the flags to make our block "discardable" */
         header = get_header( hmem );
         header->flags |= LMEM_DISCARDABLE >> 8;
         ret = hmem;
      }
      else SetLastError( ERROR_INVALID_PARAMETER );
   }
   else
   {
      if (is_pointer( hmem ))
      {
         /* reallocate fixed memory */
         if (!(flags & LMEM_MOVEABLE)) heap_flags |= HEAP_REALLOC_IN_PLACE_ONLY;
         ret = HeapReAlloc( GetProcessHeap(), heap_flags, hmem, size );
      }
      else
      {
          /* reallocate a moveable block */
          header = get_header( hmem );
          if (size != 0)
          {
              if (size <= INT_MAX - HLOCAL_STORAGE)
              {
                  if (header->ptr)
                  {
                      if ((ptr = HeapReAlloc( GetProcessHeap(), heap_flags,
                                              (char *)header->ptr - HLOCAL_STORAGE,
                                              size + HLOCAL_STORAGE )))
                      {
                          header->ptr = (char *)ptr + HLOCAL_STORAGE;
                          ret = hmem;
                      }
                  }
                  else
                  {
                      if ((ptr = HeapAlloc( GetProcessHeap(), heap_flags, size + HLOCAL_STORAGE )))
                      {
                          *(HLOCAL *)ptr = hmem;
                          header->ptr = (char *)ptr + HLOCAL_STORAGE;
                          ret = hmem;
                      }
                  }
              }
              else SetLastError( ERROR_OUTOFMEMORY );
          }
          else
          {
              if (header->lock == 0)
              {
                  if (header->ptr)
                  {
                      HeapFree( GetProcessHeap(), 0, (char *)header->ptr - HLOCAL_STORAGE );
                      header->ptr = NULL;
                  }
                  ret = hmem;
              }
              else WARN( "not freeing memory associated with locked handle\n" );
          }
      }
   }
   RtlUnlockHeap( GetProcessHeap() );
   return ret;
}


/***********************************************************************
 *           LocalUnlock   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH LocalUnlock( HLOCAL hmem )
{
    BOOL ret = FALSE;

    if (is_pointer( hmem ))
    {
        SetLastError( ERROR_NOT_LOCKED );
        return FALSE;
    }

    RtlLockHeap( GetProcessHeap() );
    __TRY
    {
        struct local_header *header = get_header( hmem );
        if (header->magic == MAGIC_LOCAL_USED)
        {
            if (header->lock)
            {
                header->lock--;
                ret = (header->lock != 0);
                if (!ret) SetLastError( NO_ERROR );
            }
            else
            {
                WARN( "%p not locked\n", hmem );
                SetLastError( ERROR_NOT_LOCKED );
            }
        }
        else
        {
            WARN( "invalid handle %p (Magic: 0x%04x)\n", hmem, header->magic );
            SetLastError( ERROR_INVALID_HANDLE );
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        WARN("(%p): Page fault occurred ! Caused by bug ?\n", hmem);
        SetLastError( ERROR_INVALID_PARAMETER );
    }
    __ENDTRY
    RtlUnlockHeap( GetProcessHeap() );
    return ret;
}


/***********************************************************************
 * Memory resource functions
 ***********************************************************************/


/***********************************************************************
 *           CreateMemoryResourceNotification   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateMemoryResourceNotification( MEMORY_RESOURCE_NOTIFICATION_TYPE type )
{
    static const WCHAR lowmemW[] =
        {'\\','K','e','r','n','e','l','O','b','j','e','c','t','s',
         '\\','L','o','w','M','e','m','o','r','y','C','o','n','d','i','t','i','o','n',0};
    static const WCHAR highmemW[] =
        {'\\','K','e','r','n','e','l','O','b','j','e','c','t','s',
         '\\','H','i','g','h','M','e','m','o','r','y','C','o','n','d','i','t','i','o','n',0};
    HANDLE ret;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;

    switch (type)
    {
    case LowMemoryResourceNotification:
        RtlInitUnicodeString( &nameW, lowmemW );
        break;
    case HighMemoryResourceNotification:
        RtlInitUnicodeString( &nameW, highmemW );
        break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    InitializeObjectAttributes( &attr, &nameW, 0, 0, NULL );
    if (!set_ntstatus( NtOpenEvent( &ret, EVENT_ALL_ACCESS, &attr ))) return 0;
    return ret;
}

/***********************************************************************
 *          QueryMemoryResourceNotification   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryMemoryResourceNotification( HANDLE handle, BOOL *state )
{
    switch (WaitForSingleObject( handle, 0 ))
    {
    case WAIT_OBJECT_0:
        *state = TRUE;
        return TRUE;
    case WAIT_TIMEOUT:
        *state = FALSE;
        return TRUE;
    }
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}


/***********************************************************************
 * Physical memory functions
 ***********************************************************************/


/***********************************************************************
 *             AllocateUserPhysicalPages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH AllocateUserPhysicalPages( HANDLE process, ULONG_PTR *pages,
                                                         ULONG_PTR *userarray )
{
    FIXME( "stub: %p %p %p\n", process, pages, userarray );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *             FreeUserPhysicalPages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FreeUserPhysicalPages( HANDLE process, ULONG_PTR *pages,
                                                     ULONG_PTR *userarray )
{
    FIXME( "stub: %p %p %p\n", process, pages, userarray );
    *pages = 0;
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *             MapUserPhysicalPages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH MapUserPhysicalPages( void *addr, ULONG_PTR page_count, ULONG_PTR *pages )
{
    FIXME( "stub: %p %lu %p\n", addr, page_count, pages );
    *pages = 0;
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 * NUMA functions
 ***********************************************************************/


/***********************************************************************
 *             AllocateUserPhysicalPagesNuma   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH AllocateUserPhysicalPagesNuma( HANDLE process, ULONG_PTR *pages,
                                                             ULONG_PTR *userarray, DWORD node )
{
    if (node) FIXME( "Ignoring preferred node %u\n", node );
    return AllocateUserPhysicalPages( process, pages, userarray );
}


/***********************************************************************
 *             CreateFileMappingNumaW   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateFileMappingNumaW( HANDLE file, LPSECURITY_ATTRIBUTES sa,
                                                        DWORD protect, DWORD size_high, DWORD size_low,
                                                        LPCWSTR name, DWORD node )
{
    if (node) FIXME( "Ignoring preferred node %u\n", node );
    return CreateFileMappingW( file, sa, protect, size_high, size_low, name );
}


/***********************************************************************
 *           GetLogicalProcessorInformation   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetLogicalProcessorInformation( SYSTEM_LOGICAL_PROCESSOR_INFORMATION *buffer,
                                                              DWORD *len )
{
    NTSTATUS status;

    if (!len)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    status = NtQuerySystemInformation( SystemLogicalProcessorInformation, buffer, *len, len );
    if (status == STATUS_INFO_LENGTH_MISMATCH) status = STATUS_BUFFER_TOO_SMALL;
    return set_ntstatus( status );
}


/***********************************************************************
 *           GetLogicalProcessorInformationEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetLogicalProcessorInformationEx( LOGICAL_PROCESSOR_RELATIONSHIP relationship,
                                            SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buffer, DWORD *len )
{
    NTSTATUS status;

    if (!len)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    status = NtQuerySystemInformationEx( SystemLogicalProcessorInformationEx, &relationship,
                                         sizeof(relationship), buffer, *len, len );
    if (status == STATUS_INFO_LENGTH_MISMATCH) status = STATUS_BUFFER_TOO_SMALL;
    return set_ntstatus( status );
}


/**********************************************************************
 *             GetNumaHighestNodeNumber   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetNumaHighestNodeNumber( ULONG *node )
{
    FIXME( "semi-stub: %p\n", node );
    *node = 0;
    return TRUE;
}


/**********************************************************************
 *             GetNumaNodeProcessorMaskEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetNumaNodeProcessorMaskEx( USHORT node, GROUP_AFFINITY *mask )
{
    FIXME( "stub: %hu %p\n", node, mask );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *             GetNumaProximityNodeEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetNumaProximityNodeEx( ULONG proximity_id, USHORT *node )
{
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *             MapViewOfFileExNuma   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH MapViewOfFileExNuma( HANDLE handle, DWORD access, DWORD offset_high,
                                                     DWORD offset_low, SIZE_T count, LPVOID addr,
                                                     DWORD node )
{
    if (node) FIXME( "Ignoring preferred node %u\n", node );
    return MapViewOfFileEx( handle, access, offset_high, offset_low, count, addr );
}


/***********************************************************************
 *             VirtualAllocExNuma   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH VirtualAllocExNuma( HANDLE process, void *addr, SIZE_T size,
                                                    DWORD type, DWORD protect, DWORD node )
{
    if (node) FIXME( "Ignoring preferred node %u\n", node );
    return VirtualAllocEx( process, addr, size, type, protect );
}
