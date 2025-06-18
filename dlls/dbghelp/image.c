/*
 * File image.c - managing images
 *
 * Copyright (C) 2004, Eric Pouech
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dbghelp_private.h"
#include "winternl.h"
#include "wine/debug.h"

/***********************************************************************
 *		GetTimestampForLoadedLibrary (DBGHELP.@)
 */
DWORD WINAPI GetTimestampForLoadedLibrary(HMODULE Module)
{
    IMAGE_NT_HEADERS*   nth = RtlImageNtHeader(Module);
    return (nth) ? nth->FileHeader.TimeDateStamp : 0;
}

#ifndef _WIN64
WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

/***********************************************************************
 *		MapDebugInformation (DBGHELP.@)
 */
PIMAGE_DEBUG_INFORMATION WINAPI MapDebugInformation(HANDLE FileHandle, PCSTR FileName,
                                                    PCSTR SymbolPath, ULONG ImageBase)
{
    FIXME("(%p, %s, %s, 0x%08lx): stub\n", FileHandle, debugstr_a(FileName), debugstr_a(SymbolPath), ImageBase);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

/***********************************************************************
 *		UnmapDebugInformation (DBGHELP.@)
 */
BOOL WINAPI UnmapDebugInformation(PIMAGE_DEBUG_INFORMATION DebugInfo)
{
    FIXME("(%p): stub\n", DebugInfo);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif
