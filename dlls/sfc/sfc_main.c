/*
 * Implementation of the System File Checker (Windows File Protection)
 *
 * Copyright 2006 Detlef Riekenberg
 *
 * This file contains only stubs to get the localspl.dll up and running
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
#include "sfc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sfc);

/******************************************************************
 *              SfcIsFileProtected     [SFC.@]
 *
 * Check, if the given File is protected by the System
 *
 * PARAMS
 *  RpcHandle    [I] This must be NULL
 *  ProtFileName [I] Filename with Path to check
 *
 * RETURNS
 *  Failure: FALSE with GetLastError() != ERROR_FILE_NOT_FOUND
 *  Success: TRUE, when the File is Protected
 *           FALSE with GetLastError() == ERROR_FILE_NOT_FOUND,
 *           when the File is not Protected
 *
 *
 * BUGS
 *  We return always the Result for: "File is not Protected"
 *
 */
BOOL WINAPI SfcIsFileProtected(HANDLE RpcHandle, LPCWSTR ProtFileName)
{
    static BOOL reported = FALSE;

    if (reported) {
        TRACE("(%p, %s) stub\n", RpcHandle, debugstr_w(ProtFileName));
    }
    else
    {
        FIXME("(%p, %s) stub\n", RpcHandle, debugstr_w(ProtFileName));
        reported = TRUE;
    }

    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}
