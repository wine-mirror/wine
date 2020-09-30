/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 * Copyright 1999 Peter Ganten
 * Copyright 2002 Martin Wilck
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
#include <string.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "winternl.h"
#include "wine/exception.h"

#include "kernel_private.h"


/***********************************************************************
 *              GetComputerNameW         (KERNEL32.@)
 */
BOOL WINAPI GetComputerNameW(LPWSTR name,LPDWORD size)
{
    BOOL ret = GetComputerNameExW( ComputerNameNetBIOS, name, size );
    if (!ret && GetLastError() == ERROR_MORE_DATA) SetLastError( ERROR_BUFFER_OVERFLOW );
    return ret;
}

/***********************************************************************
 *              GetComputerNameA         (KERNEL32.@)
 */
BOOL WINAPI GetComputerNameA(LPSTR name, LPDWORD size)
{
    WCHAR nameW[ MAX_COMPUTERNAME_LENGTH + 1 ];
    DWORD sizeW = MAX_COMPUTERNAME_LENGTH + 1;
    unsigned int len;
    BOOL ret;

    if ( !GetComputerNameW (nameW, &sizeW) ) return FALSE;

    len = WideCharToMultiByte ( CP_ACP, 0, nameW, -1, NULL, 0, NULL, 0 );
    /* for compatibility with Win9x */
    __TRY
    {
        if ( *size < len )
        {
            *size = len;
            SetLastError( ERROR_BUFFER_OVERFLOW );
            ret = FALSE;
        }
        else
        {
            WideCharToMultiByte ( CP_ACP, 0, nameW, -1, name, len, NULL, 0 );
            *size = len - 1;
            ret = TRUE;
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        ret = FALSE;
    }
    __ENDTRY

    return ret;
}

/***********************************************************************
 *              DnsHostnameToComputerNameA         (KERNEL32.@)
 */
BOOL WINAPI DnsHostnameToComputerNameA(LPCSTR hostname,
    LPSTR computername, LPDWORD size)
{
    WCHAR *hostW, nameW[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD len;
    BOOL ret;

    if (!hostname || !size) return FALSE;
    len = MultiByteToWideChar( CP_ACP, 0, hostname, -1, NULL, 0 );
    if (!(hostW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return FALSE;
    MultiByteToWideChar( CP_ACP, 0, hostname, -1, hostW, len );
    len = ARRAY_SIZE(nameW);
    if ((ret = DnsHostnameToComputerNameW( hostW, nameW, &len )))
    {
        if (!computername || !WideCharToMultiByte( CP_ACP, 0, nameW, -1, computername, *size, NULL, NULL ))
            *size = WideCharToMultiByte( CP_ACP, 0, nameW, -1, NULL, 0, NULL, NULL );
        else
            *size = strlen(computername);
    }
    HeapFree( GetProcessHeap(), 0, hostW );
    return TRUE;
}

/***********************************************************************
 *              DnsHostnameToComputerNameW         (KERNEL32.@)
 */
BOOL WINAPI DnsHostnameToComputerNameW(LPCWSTR hostname,
    LPWSTR computername, LPDWORD size)
{
    return DnsHostnameToComputerNameExW( hostname, computername, size );
}
