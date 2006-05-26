/*
 * Full Pointer Translation Routines
 *
 * Copyright 2006 Robert Shearman
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

#include "windef.h"
#include "winbase.h"
#include "rpc.h"
#include "rpcndr.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

PFULL_PTR_XLAT_TABLES WINAPI NdrFullPointerXlatInit(unsigned long NumberOfPointers,
                                                    XLAT_SIDE XlatSide)
{
    unsigned long NumberOfBuckets = ((NumberOfPointers + 3) & 4) - 1;
    PFULL_PTR_XLAT_TABLES pXlatTables = HeapAlloc(GetProcessHeap(), 0, sizeof(*pXlatTables));

    TRACE("(%ld, %d)\n", NumberOfPointers, XlatSide);

    pXlatTables->RefIdToPointer.XlatTable =
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(void *) * NumberOfPointers);
    pXlatTables->RefIdToPointer.StateTable =
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(unsigned char) * NumberOfPointers);
    pXlatTables->RefIdToPointer.NumberOfEntries = NumberOfPointers;

    TRACE("NumberOfBuckets = %ld\n", NumberOfBuckets);
    pXlatTables->PointerToRefId.XlatTable =
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(PFULL_PTR_TO_REFID_ELEMENT) * NumberOfBuckets);
    pXlatTables->PointerToRefId.NumberOfBuckets = NumberOfBuckets;
    pXlatTables->PointerToRefId.HashMask = NumberOfBuckets - 1;

    pXlatTables->NextRefId = 1;
    pXlatTables->XlatSide = XlatSide;

    return pXlatTables;
}

void WINAPI NdrFullPointerXlatFree(PFULL_PTR_XLAT_TABLES pXlatTables)
{
    TRACE("(%p)\n", pXlatTables);

    HeapFree(GetProcessHeap(), 0, pXlatTables->RefIdToPointer.XlatTable);
    HeapFree(GetProcessHeap(), 0, pXlatTables->RefIdToPointer.StateTable);
    HeapFree(GetProcessHeap(), 0, pXlatTables->PointerToRefId.XlatTable);

    HeapFree(GetProcessHeap(), 0, pXlatTables);
}

static void expand_pointer_table_if_necessary(PFULL_PTR_XLAT_TABLES pXlatTables, unsigned long RefId)
{
    if (RefId >= pXlatTables->RefIdToPointer.NumberOfEntries)
    {
        pXlatTables->RefIdToPointer.NumberOfEntries = RefId * 2;
        pXlatTables->RefIdToPointer.XlatTable =
            HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                pXlatTables->RefIdToPointer.XlatTable,
                sizeof(void *) * pXlatTables->RefIdToPointer.NumberOfEntries);
        pXlatTables->RefIdToPointer.StateTable =
            HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                pXlatTables->RefIdToPointer.StateTable,
                sizeof(unsigned char) * pXlatTables->RefIdToPointer.NumberOfEntries);

        if (!pXlatTables->RefIdToPointer.XlatTable || !pXlatTables->RefIdToPointer.StateTable)
            pXlatTables->RefIdToPointer.NumberOfEntries = 0;
    }
}

int WINAPI NdrFullPointerQueryPointer(PFULL_PTR_XLAT_TABLES pXlatTables,
                                      void *pPointer, unsigned char QueryType,
                                      unsigned long *pRefId )
{
    unsigned long Hash = 0;
    int i;
    PFULL_PTR_TO_REFID_ELEMENT XlatTableEntry;

    TRACE("(%p, %p, %d, %p)\n", pXlatTables, pPointer, QueryType, pRefId);

    if (!pPointer)
    {
        *pRefId = 0;
        return 1;
    }

    /* simple hashing algorithm, don't know whether it matches native */
    for (i = 0; i < sizeof(pPointer); i++)
        Hash = (Hash * 3) ^ ((unsigned char *)&pPointer)[i];

    TRACE("pXlatTables->PointerToRefId.XlatTable = %p\n", pXlatTables->PointerToRefId.XlatTable);
    TRACE("Hash = 0x%lx, pXlatTables->PointerToRefId.HashMask = 0x%lx\n", Hash, pXlatTables->PointerToRefId.HashMask);
    XlatTableEntry = pXlatTables->PointerToRefId.XlatTable[Hash & pXlatTables->PointerToRefId.HashMask];
    for (; XlatTableEntry; XlatTableEntry = XlatTableEntry->Next)
        if (pPointer == XlatTableEntry->Pointer)
        {
            *pRefId = XlatTableEntry->RefId;
            if (pXlatTables->XlatSide == XLAT_SERVER)
                return XlatTableEntry->State;
            else
                return 0;
        }

    XlatTableEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(*XlatTableEntry));
    XlatTableEntry->Next = pXlatTables->PointerToRefId.XlatTable[Hash & pXlatTables->PointerToRefId.HashMask];
    XlatTableEntry->Pointer = pPointer;
    XlatTableEntry->RefId = *pRefId = pXlatTables->NextRefId++;
    XlatTableEntry->State = QueryType;
    pXlatTables->PointerToRefId.XlatTable[Hash & pXlatTables->PointerToRefId.HashMask] = XlatTableEntry;

    /* FIXME: insert pointer into mapping table */

    return 0;
}

int WINAPI NdrFullPointerQueryRefId(PFULL_PTR_XLAT_TABLES pXlatTables,
                                    unsigned long RefId,
                                    unsigned char QueryType, void **ppPointer)
{
    TRACE("(%p, 0x%lx, %d, %p)\n", pXlatTables, RefId, QueryType, ppPointer);

    expand_pointer_table_if_necessary(pXlatTables, RefId);

    *ppPointer = pXlatTables->RefIdToPointer.XlatTable[RefId];
    return 0;
}

void WINAPI NdrFullPointerInsertRefId(PFULL_PTR_XLAT_TABLES pXlatTables,
                                      unsigned long RefId, void *pPointer)
{
    FIXME("(%p, 0x%lx, %p)\n", pXlatTables, RefId, pPointer);
}

int WINAPI NdrFullPointerFree(PFULL_PTR_XLAT_TABLES pXlatTables, void *Pointer)
{
    FIXME("(%p, %p)\n", pXlatTables, Pointer);

    return 10;
}
