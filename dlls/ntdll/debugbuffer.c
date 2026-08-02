/*
 * NtDll debug buffer functions
 *
 * Copyright 2004 Raphael Junqueira
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(debug_buffer);

static void dump_DEBUG_MODULE_INFORMATION(const DEBUG_MODULE_INFORMATION *iBuf)
{
  TRACE( "MODULE_INFORMATION:%p\n", iBuf );
  if (NULL == iBuf) return ;
  TRACE( "Base:%lx\n", iBuf->Base );
  TRACE( "Size:%lx\n", iBuf->Size );
  TRACE( "Flags:%lx\n", iBuf->Flags );
}

static void dump_DEBUG_HEAP_INFORMATION(const DEBUG_HEAP_INFORMATION *iBuf)
{
  TRACE( "HEAP_INFORMATION:%p\n", iBuf );
  if (NULL == iBuf) return ;
  TRACE( "Base:%lx\n", iBuf->Base );
  TRACE( "Flags:%lx\n", iBuf->Flags );
}

static void dump_DEBUG_LOCK_INFORMATION(const DEBUG_LOCK_INFORMATION *iBuf)
{
  TRACE( "LOCK_INFORMATION:%p\n", iBuf );

  if (NULL == iBuf) return ;

  TRACE( "Address:%p\n", iBuf->Address );
  TRACE( "Type:%d\n", iBuf->Type );
  TRACE( "CreatorBackTraceIndex:%d\n", iBuf->CreatorBackTraceIndex );
  TRACE( "OwnerThreadId:%lx\n", iBuf->OwnerThreadId );
  TRACE( "ActiveCount:%ld\n", iBuf->ActiveCount );
  TRACE( "ContentionCount:%ld\n", iBuf->ContentionCount );
  TRACE( "EntryCount:%ld\n", iBuf->EntryCount );
  TRACE( "RecursionCount:%ld\n", iBuf->RecursionCount );
  TRACE( "NumberOfSharedWaiters:%ld\n", iBuf->NumberOfSharedWaiters );
  TRACE( "NumberOfExclusiveWaiters:%ld\n", iBuf->NumberOfExclusiveWaiters );
}

static void dump_DEBUG_BUFFER(const DEBUG_BUFFER *iBuf)
{
  if (NULL == iBuf) return ;
  TRACE( "SectionHandle:%p\n", iBuf->SectionHandle);
  TRACE( "SectionBase:%p\n", iBuf->SectionBase);
  TRACE( "RemoteSectionBase:%p\n", iBuf->RemoteSectionBase);
  TRACE( "SectionBaseDelta:%lx\n", iBuf->SectionBaseDelta);
  TRACE( "EventPairHandle:%p\n", iBuf->EventPairHandle);
  TRACE( "RemoteThreadHandle:%p\n", iBuf->RemoteThreadHandle);
  TRACE( "InfoClassMask:%lx\n", iBuf->InfoClassMask);
  TRACE( "SizeOfInfo:%Iu\n", iBuf->SizeOfInfo);
  TRACE( "AllocatedSize:%Iu\n", iBuf->AllocatedSize);
  TRACE( "SectionSize:%ld\n", iBuf->SectionSize);
  TRACE( "BackTraceInfo:%p\n", iBuf->BackTraceInformation);
  dump_DEBUG_MODULE_INFORMATION(iBuf->ModuleInformation);
  dump_DEBUG_HEAP_INFORMATION(iBuf->HeapInformation);
  dump_DEBUG_LOCK_INFORMATION(iBuf->LockInformation);
}

PDEBUG_BUFFER WINAPI RtlCreateQueryDebugBuffer(IN ULONG iSize, IN BOOLEAN iEventPair) 
{
   PDEBUG_BUFFER oBuf;
   FIXME("(%ld, %d): stub\n", iSize, iEventPair);
   if (iSize < sizeof(DEBUG_BUFFER)) {
     iSize = sizeof(DEBUG_BUFFER);
   }
   oBuf = RtlAllocateHeap(GetProcessHeap(), 0, iSize);
   memset(oBuf, 0, iSize);
   FIXME("(%ld, %d): returning %p\n", iSize, iEventPair, oBuf);
   return oBuf;
}

NTSTATUS WINAPI RtlDestroyQueryDebugBuffer(IN PDEBUG_BUFFER iBuf) 
{
   NTSTATUS nts = STATUS_SUCCESS;
   FIXME("(%p): stub\n", iBuf);
   if (NULL != iBuf) {
     RtlFreeHeap(GetProcessHeap(), 0, iBuf->ModuleInformation);
     RtlFreeHeap(GetProcessHeap(), 0, iBuf->HeapInformation);
     RtlFreeHeap(GetProcessHeap(), 0, iBuf->LockInformation);
     RtlFreeHeap(GetProcessHeap(), 0, iBuf);
   }
   return nts;
}

NTSTATUS WINAPI RtlQueryProcessDebugInformation(IN ULONG iProcessId, IN ULONG iDebugInfoMask, IN OUT PDEBUG_BUFFER iBuf) 
{
    CLIENT_ID cid;
    NTSTATUS status;
    HANDLE process;

    cid.UniqueProcess = ULongToHandle( iProcessId );
    cid.UniqueThread = 0;

    if ((status = NtOpenProcess( &process, PROCESS_QUERY_LIMITED_INFORMATION, NULL, &cid ))) return status;
    NtClose( process );

   FIXME("(%ld, %lx, %p): stub\n", iProcessId, iDebugInfoMask, iBuf);
   iBuf->InfoClassMask = iDebugInfoMask;

   if (iDebugInfoMask & PDI_MODULES) {
     PDEBUG_MODULE_INFORMATION info = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(DEBUG_MODULE_INFORMATION));
     memset(info, 0, sizeof(DEBUG_MODULE_INFORMATION));
     iBuf->ModuleInformation = info;
   }
   if (iDebugInfoMask & PDI_HEAPS) {
     PDEBUG_HEAP_INFORMATION info = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(DEBUG_HEAP_INFORMATION));
     memset(info, 0, sizeof(DEBUG_HEAP_INFORMATION));
     if (iDebugInfoMask & PDI_HEAP_TAGS) {
     }
     if (iDebugInfoMask & PDI_HEAP_BLOCKS) {
     }
     iBuf->HeapInformation = info;
   }
   if (iDebugInfoMask & PDI_LOCKS) {
     PDEBUG_LOCK_INFORMATION info = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(DEBUG_LOCK_INFORMATION));
     memset(info, 0, sizeof(DEBUG_LOCK_INFORMATION));
     iBuf->LockInformation = info;
   }
   TRACE("returns:%p\n", iBuf);
   dump_DEBUG_BUFFER(iBuf);
   return status;
}
