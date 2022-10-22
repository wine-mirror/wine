/*
 * Print processor implementation.
 *
 * Copyright 2022 Piotr Caban for CodeWeavers
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

#include <stdio.h>

#include <windows.h>
#include <winspool.h>
#include <ddk/winsplp.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

BOOL WINAPI EnumPrintProcessorDatatypesW(WCHAR *server, WCHAR *name, DWORD level,
        BYTE *datatypes, DWORD size, DWORD *needed, DWORD *no)
{
    FIXME("%s, %s, %ld, %p, %ld, %p, %p\n", debugstr_w(server), debugstr_w(name),
            level, datatypes, size, needed, no);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

HANDLE WINAPI OpenPrintProcessor(WCHAR *port, PRINTPROCESSOROPENDATA *open_data)
{
    FIXME("%s, %p\n", debugstr_w(port), open_data);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

BOOL WINAPI PrintDocumentOnPrintProcessor(HANDLE pp, WCHAR *doc_name)
{
    FIXME("%p, %s\n", pp, debugstr_w(doc_name));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI ControlPrintProcessor(HANDLE pp, DWORD cmd)
{
    FIXME("%p, %ld\n", pp, cmd);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI ClosePrintProcessor(HANDLE pp)
{
    FIXME("%p\n", pp);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
