/*
 * Implementation of the System File Checker (Windows File Protection)
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
#include "winreg.h"
#include "sfc.h"
#include "srrestoreptapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sfc);

/******************************************************************
 *      SfcGetNextProtectedFile     [sfc_os.@]
 */
BOOL WINAPI SfcGetNextProtectedFile(HANDLE handle, PROTECTED_FILE_DATA *data)
{
    FIXME("%p %p\n", handle, data);

    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}

/******************************************************************
 *              SfcIsFileProtected     [sfc_os.@]
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

/******************************************************************
 *              SfcIsKeyProtected     [sfc_os.@]
 *
 * Check, if the given Registry Key is protected by the System
 *
 * PARAMS
 *  hKey          [I] Handle to the root registry key
 *  lpSubKey      [I] Name of the subkey to check
 *  samDesired    [I] The Registry View to Examine (32 or 64 bit)
 *
 * RETURNS
 *  Failure: FALSE with GetLastError() != ERROR_FILE_NOT_FOUND
 *  Success: TRUE, when the Key is Protected
 *           FALSE with GetLastError() == ERROR_FILE_NOT_FOUND,
 *           when the Key is not Protected
 *
 */
BOOL WINAPI SfcIsKeyProtected(HKEY hKey, LPCWSTR lpSubKey, REGSAM samDesired)
{
    static BOOL reported = FALSE;

    if (reported) {
        TRACE("(%p, %s) stub\n", hKey, debugstr_w(lpSubKey));
    }
    else
    {
        FIXME("(%p, %s) stub\n", hKey, debugstr_w(lpSubKey));
        reported = TRUE;
    }

    if( !hKey ) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

DWORD WINAPI SfcConnectToServer(DWORD unknown)
{
    FIXME("%lx\n", unknown);
    return 0;
}

BOOL WINAPI SfpVerifyFile(LPCSTR filename, LPSTR error, DWORD size)
{
    FIXME("%s: stub\n", debugstr_a(filename));
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

BOOL WINAPI SRSetRestorePointA(RESTOREPOINTINFOA *restorepoint, STATEMGRSTATUS *status)
{
    FIXME("%p %p\n", restorepoint, status);
    status->nStatus = ERROR_SUCCESS;
    return FALSE;
}

BOOL WINAPI SRSetRestorePointW(RESTOREPOINTINFOW *restorepoint, STATEMGRSTATUS *status)
{
    FIXME("%p %p\n", restorepoint, status);
    status->nStatus = ERROR_SUCCESS;
    return FALSE;
}
