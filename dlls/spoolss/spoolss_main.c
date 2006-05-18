/*
 * Implementation of the Spooler-Service helper DLL
 *
 * Copyright 2006 Detlef Riekenberg
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
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(spoolss);


/******************************************************************
 *
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %ld, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
        case DLL_WINE_PREATTACH:
            return FALSE;  /* prefer native version */
        case DLL_PROCESS_ATTACH: {
            DisableThreadLibraryCalls(hinstDLL);
            break;
        }
    }
    return TRUE;
}

/******************************************************************
 *   DllAllocSplMem   [SPOOLSS.@]
 *
 * Allocate cleared memory from the spooler heap
 *
 * PARAMS
 *  size [I] Number of bytes to allocate
 *
 * RETURNS
 *  Failure: NULL
 *  Success: PTR to the allocated memory
 *
 * NOTES
 *  We use the process heap (Windows use a separate spooler heap)
 *
 */
LPVOID WINAPI DllAllocSplMem(DWORD size)
{
    LPVOID  res;

    res = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    TRACE("(%ld) => %p\n", size, res);
    return res;
}

/******************************************************************
 *   DllFreeSplMem   [SPOOLSS.@]
 *
 * Free the allocated spooler memory
 *
 * PARAMS
 *  memory [I] PTR to the memory allocated by DllAllocSplMem
 *
 * RETURNS
 *  Failure: FALSE
 *  Success: TRUE
 *
 * NOTES
 *  We use the process heap (Windows use a separate spooler heap)
 *
 */

BOOL WINAPI DllFreeSplMem(LPBYTE memory)
{
    TRACE("(%p)\n", memory);
    return HeapFree(GetProcessHeap(), 0, memory);
}
