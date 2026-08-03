/*
 *
 * Copyright 2008 Alistair Leslie-Hughes
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
#include "winbase.h"
#include "winternl.h"
#include "wine/debug.h"

#include "slpublic.h"
#include "slerror.h"

WINE_DEFAULT_DEBUG_CHANNEL(slc);

HRESULT WINAPI SLGetWindowsInformation(LPCWSTR name, SLDATATYPE *type, UINT *val, LPBYTE *size)
{
    FIXME("(%s %p %p %p) stub\n", debugstr_w(name), type, val, size );

    return SL_E_RIGHT_NOT_GRANTED;
}

HRESULT WINAPI SLGetWindowsInformationDWORD(LPCWSTR lpszValueName, LPDWORD pdwValue)
{
    UNICODE_STRING nameW;
    NTSTATUS status;
    ULONG type, len;

    TRACE("(%s)\n", debugstr_w(lpszValueName) );

    if (!lpszValueName || !pdwValue)
        return E_INVALIDARG;
    if (!lpszValueName[0])
        return SL_E_RIGHT_NOT_GRANTED;

    RtlInitUnicodeString( &nameW, lpszValueName );
    status = NtQueryLicenseValue( &nameW, &type, pdwValue, sizeof(DWORD), &len );

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        return SL_E_VALUE_NOT_FOUND;
    if ((!status || status == STATUS_BUFFER_TOO_SMALL) && (type != REG_DWORD))
        return SL_E_DATATYPE_MISMATCHED;

    return status ? E_FAIL : S_OK;
}
