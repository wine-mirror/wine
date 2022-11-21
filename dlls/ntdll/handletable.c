/*
 * Handle Tables
 *
 * Copyright (C) 2004 Robert Shearman
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

/**************************************************************************
 *	RtlInitializeHandleTable   (NTDLL.@)
 *
 * Initializes a handle table.
 *
 * PARAMS
 *  MaxHandleCount [I] The maximum number of handles the handle table will support.
 *  HandleSize     [I] The size of each handle.
 *  HandleTable    [I/O] The handle table.
 *
 * RETURNS
 *  Nothing.
 *
 * SEE
 *  RtlDestroyHandleTable().
 */
void WINAPI RtlInitializeHandleTable(ULONG MaxHandleCount, ULONG HandleSize, RTL_HANDLE_TABLE * HandleTable)
{
    TRACE("(%lu, %lu, %p)\n", MaxHandleCount, HandleSize, HandleTable);

    memset(HandleTable, 0, sizeof(*HandleTable));
    HandleTable->MaxHandleCount = MaxHandleCount;
    HandleTable->HandleSize = HandleSize;
}

/**************************************************************************
 *	RtlDestroyHandleTable   (NTDLL.@)
 *
 * Destroys a handle table and frees associated resources.
 *
 * PARAMS
 *  HandleTable    [I] The handle table.
 *
 * RETURNS
 *  Any status code returned by NtFreeVirtualMemory().
 *
 * NOTES
 *  The native version of this API doesn't free the virtual memory that has
 *  been previously reserved, only the committed memory. There is no harm
 *  in also freeing the reserved memory because it won't have been handed out
 *  to any callers. I believe it is "more polite" to free everything.
 *
 * SEE
 *  RtlInitializeHandleTable().
 */
NTSTATUS WINAPI RtlDestroyHandleTable(RTL_HANDLE_TABLE * HandleTable)
{
    SIZE_T Size = 0;

    TRACE("(%p)\n", HandleTable);

    /* native version only releases committed memory, but we also release reserved */
    return NtFreeVirtualMemory(
        NtCurrentProcess(),
        &HandleTable->FirstHandle,
        &Size,
        MEM_RELEASE);
}

/**************************************************************************
 *	RtlpAllocateSomeHandles   (internal)
 *
 * Reserves memory for the handles if not previously done and commits memory
 * for a batch of handles if none are free and adds them to the free list.
 *
 * PARAMS
 *  HandleTable    [I/O] The handle table.
 *
 * RETURNS
 *  NTSTATUS code.
 */
static NTSTATUS RtlpAllocateSomeHandles(RTL_HANDLE_TABLE * HandleTable)
{
    NTSTATUS status;
    if (!HandleTable->FirstHandle)
    {
        PVOID FirstHandleAddr = NULL;
        SIZE_T MaxSize = HandleTable->MaxHandleCount * HandleTable->HandleSize;

        /* reserve memory for the handles, but don't commit it yet because we
         * probably won't use most of it and it will use up physical memory */
        status = NtAllocateVirtualMemory(
            NtCurrentProcess(),
            &FirstHandleAddr,
            0,
            &MaxSize,
            MEM_RESERVE,
            PAGE_READWRITE);
        if (status != STATUS_SUCCESS)
            return status;
        HandleTable->FirstHandle = FirstHandleAddr;
        HandleTable->ReservedMemory = HandleTable->FirstHandle;
        HandleTable->MaxHandle = (char *)HandleTable->FirstHandle + MaxSize;
    }
    if (!HandleTable->NextFree)
    {
        SIZE_T Offset, CommitSize = 4096; /* one page */
        RTL_HANDLE * FreeHandle = NULL;
        PVOID NextAvailAddr = HandleTable->ReservedMemory;

        if (HandleTable->ReservedMemory >= HandleTable->MaxHandle)
            return STATUS_NO_MEMORY; /* the handle table is completely full */

        status = NtAllocateVirtualMemory(
            NtCurrentProcess(),
            &NextAvailAddr,
            0,
            &CommitSize,
            MEM_COMMIT,
            PAGE_READWRITE);
        if (status != STATUS_SUCCESS)
            return status;

        for (Offset = 0; Offset < CommitSize; Offset += HandleTable->HandleSize)
        {
            /* make sure we don't go over handle limit, even if we can
             * because of rounding of the table size up to the next page
             * boundary */
            if ((char *)HandleTable->ReservedMemory + Offset >= (char *)HandleTable->MaxHandle)
                break;

            FreeHandle = (RTL_HANDLE *)((char *)HandleTable->ReservedMemory + Offset);

            FreeHandle->Next = (RTL_HANDLE *)((char *)HandleTable->ReservedMemory + 
                Offset + HandleTable->HandleSize);
        }

        /* shouldn't happen because we already test for this above, but
         * handle it just in case */
        if (!FreeHandle)
            return STATUS_NO_MEMORY;

        /* set the last handle's Next pointer to NULL so that when we run
         * out of free handles we trigger another commit of memory and
         * initialize the free pointers */
        FreeHandle->Next = NULL;

        HandleTable->NextFree = HandleTable->ReservedMemory;

        HandleTable->ReservedMemory = (char *)HandleTable->ReservedMemory + CommitSize;
    }
    return STATUS_SUCCESS;
}

/**************************************************************************
 *	RtlAllocateHandle   (NTDLL.@)
 *
 * Allocates a handle from the handle table.
 *
 * PARAMS
 *  HandleTable    [I/O] The handle table.
 *  HandleIndex    [O] Index of the handle returned. Optional.
 *
 * RETURNS
 *  Success: Pointer to allocated handle.
 *  Failure: NULL.
 *
 * NOTES
 *  A valid handle must have the bit set as indicated in the code below 
 *  otherwise subsequent RtlIsValidHandle() calls will fail.
 *
 *  static inline void RtlpMakeHandleAllocated(RTL_HANDLE * Handle)
 *  {
 *    ULONG_PTR *AllocatedBit = (ULONG_PTR *)(&Handle->Next);
 *    *AllocatedBit = *AllocatedBit | 1;
 *  }
 *
 * SEE
 *  RtlFreeHandle().
 */
RTL_HANDLE * WINAPI RtlAllocateHandle(RTL_HANDLE_TABLE * HandleTable, ULONG * HandleIndex)
{
    RTL_HANDLE * ret;

    TRACE("(%p, %p)\n", HandleTable, HandleIndex);

    if (!HandleTable->NextFree && RtlpAllocateSomeHandles(HandleTable) != STATUS_SUCCESS)
        return NULL;

    ret = HandleTable->NextFree;
    HandleTable->NextFree = ret->Next;

    if (HandleIndex)
        *HandleIndex = (ULONG)(((PCHAR)ret - (PCHAR)HandleTable->FirstHandle) / HandleTable->HandleSize);

    return ret;
}

/**************************************************************************
 *	RtlFreeHandle   (NTDLL.@)
 *
 * Frees an allocated handle.
 *
 * PARAMS
 *  HandleTable    [I/O] The handle table.
 *  Handle         [I] The handle to be freed.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 *
 * SEE
 *  RtlAllocateHandle().
 */
BOOLEAN WINAPI RtlFreeHandle(RTL_HANDLE_TABLE * HandleTable, RTL_HANDLE * Handle)
{
    TRACE("(%p, %p)\n", HandleTable, Handle);
    /* NOTE: we don't validate the handle and we don't make Handle->Next even
     * again to signal that it is no longer in user - that is done as a side
     * effect of setting Handle->Next to the previously next free handle in
     * the handle table */
    memset(Handle, 0, HandleTable->HandleSize);
    Handle->Next = HandleTable->NextFree;
    HandleTable->NextFree = Handle;
    return TRUE;
}

/**************************************************************************
 *	RtlIsValidHandle   (NTDLL.@)
 *
 * Determines whether a handle is valid or not.
 *
 * PARAMS
 *  HandleTable    [I] The handle table.
 *  Handle         [I] The handle to be tested.
 *
 * RETURNS
 *  Valid: TRUE.
 *  Invalid: FALSE.
 */
BOOLEAN WINAPI RtlIsValidHandle(const RTL_HANDLE_TABLE * HandleTable, const RTL_HANDLE * Handle)
{
    TRACE("(%p, %p)\n", HandleTable, Handle);
    /* make sure handle is within used region and that it is aligned on
     * a HandleTable->HandleSize boundary and that Handle->Next is odd,
     * indicating that the handle is active */
    if ((Handle >= (RTL_HANDLE *)HandleTable->FirstHandle) &&
      (Handle < (RTL_HANDLE *)HandleTable->ReservedMemory) &&
      !((ULONG_PTR)Handle & (HandleTable->HandleSize - 1)) &&
      ((ULONG_PTR)Handle->Next & 1))
        return TRUE;
    else
        return FALSE;
}

/**************************************************************************
 *	RtlIsValidIndexHandle   (NTDLL.@)
 *
 * Determines whether a handle index is valid or not.
 *
 * PARAMS
 *  HandleTable    [I] The handle table.
 *  Index          [I] The index of the handle to be tested.
 *  ValidHandle    [O] The handle Index refers to.
 *
 * RETURNS
 *  Valid: TRUE.
 *  Invalid: FALSE.
 */
BOOLEAN WINAPI RtlIsValidIndexHandle(const RTL_HANDLE_TABLE * HandleTable, ULONG Index, RTL_HANDLE ** ValidHandle)
{
    RTL_HANDLE * Handle;

    TRACE("(%p, %lu, %p)\n", HandleTable, Index, ValidHandle);
    Handle = (RTL_HANDLE *)
        ((char *)HandleTable->FirstHandle + Index * HandleTable->HandleSize);

    if (RtlIsValidHandle(HandleTable, Handle))
    {
        *ValidHandle = Handle;
        return TRUE;
    }
    return FALSE;
}
