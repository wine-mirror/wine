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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "winternl.h"
#include "wine/unicode.h"
#include "wine/exception.h"
#include "wine/debug.h"

#include "kernel_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(computername);

/* Registry key and value names */
static const WCHAR ComputerW[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                  'M','a','c','h','i','n','e','\\',
                                  'S','y','s','t','e','m','\\',
                                  'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                  'C','o','n','t','r','o','l','\\',
                                  'C','o','m','p','u','t','e','r','N','a','m','e',0};
static const WCHAR ActiveComputerNameW[] =   {'A','c','t','i','v','e','C','o','m','p','u','t','e','r','N','a','m','e',0};
static const WCHAR ComputerNameW[] = {'C','o','m','p','u','t','e','r','N','a','m','e',0};

static const WCHAR default_ComputerName[] = {'W','I','N','E',0};

/*********************************************************************** 
 *                    dns_gethostbyname (INTERNAL)
 *
 *  From hostname(1):
 *  "The FQDN is the name gethostbyname(2) returns for the host name returned by gethostname(2)."
 *
 *  Wine can use this technique only if the thread-safe gethostbyname_r is available.
 */
static void dns_gethostbyname ( char *name, int size )
{
#ifdef HAVE_LINUX_GETHOSTBYNAME_R_6
    struct hostent* host = NULL;
    char *extrabuf;
    int ebufsize = 1024;
    struct hostent hostentry;
    int locerr = ENOBUFS, res;

    for (;;)
    {
        if (!(extrabuf = HeapAlloc( GetProcessHeap(), 0, ebufsize ))) return;
        res = gethostbyname_r ( name, &hostentry, extrabuf, ebufsize, &host, &locerr );
        if( res != ERANGE ) break;
        ebufsize *= 2;
        HeapFree( GetProcessHeap(), 0, extrabuf );
    }

    if ( res )
        WARN ("Error in gethostbyname_r %d (%d)\n", res, locerr);
    else if ( !host )
        WARN ("gethostbyname_r returned NULL host, locerr = %d\n", locerr);
    else
        if (strlen( host->h_name ) < size) strcpy( name, host->h_name );

    HeapFree( GetProcessHeap(), 0, extrabuf );
#endif
}

/*********************************************************************** 
 *                     dns_fqdn (INTERNAL)
 */
static BOOL dns_fqdn ( char *name, int size )
{
    if (gethostname( name, size ))
    {
        switch( errno )
        {
        case ENAMETOOLONG:
            SetLastError ( ERROR_MORE_DATA );
            break;
        default:
            SetLastError ( ERROR_INVALID_PARAMETER );
            break;
        }
        return FALSE;
    }
    dns_gethostbyname( name, size );
    return TRUE;
}

/*********************************************************************** 
 *                      COMPUTERNAME_Init    (INTERNAL)
 */
void COMPUTERNAME_Init (void)
{
    HANDLE hkey = INVALID_HANDLE_VALUE, hsubkey = INVALID_HANDLE_VALUE;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    char buf[offsetof( KEY_VALUE_PARTIAL_INFORMATION, Data ) + (MAX_COMPUTERNAME_LENGTH + 1) * sizeof( WCHAR )];
    DWORD len = sizeof( buf );
    const WCHAR *computer_name = (WCHAR *)(buf + offsetof( KEY_VALUE_PARTIAL_INFORMATION, Data ));
    NTSTATUS st = STATUS_INTERNAL_ERROR;
    char hbuf[256];
    WCHAR *dot, bufW[256];

    if (dns_fqdn( hbuf, sizeof(hbuf) ))
    {
        MultiByteToWideChar( CP_UNIXCP, 0, hbuf, -1, bufW, ARRAY_SIZE(bufW) );
        dot = strchrW( bufW, '.' );
        if (dot) *dot++ = 0;
        else dot = bufW + strlenW(bufW);
        SetComputerNameExW( ComputerNamePhysicalDnsDomain, dot );
        SetComputerNameExW( ComputerNamePhysicalDnsHostname, bufW );
    }

    TRACE("(void)\n");
    InitializeObjectAttributes( &attr, &nameW, 0, 0, NULL );
    RtlInitUnicodeString( &nameW, ComputerW );
    if ( ( st = NtCreateKey( &hkey, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL ) ) != STATUS_SUCCESS )
        goto out;
    
    attr.RootDirectory = hkey;
    RtlInitUnicodeString( &nameW, ComputerNameW );
    if ( (st = NtCreateKey( &hsubkey, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL ) ) != STATUS_SUCCESS )
        goto out;
    
    st = NtQueryValueKey( hsubkey, &nameW, KeyValuePartialInformation, buf, len, &len );

    if ( st != STATUS_SUCCESS)
    {
        computer_name = default_ComputerName;
        len = sizeof(default_ComputerName);
    }
    else
    {
        len = (len - offsetof( KEY_VALUE_PARTIAL_INFORMATION, Data ));
    }

    NtClose( hsubkey );
    TRACE(" ComputerName: %s (%u)\n", debugstr_w (computer_name), len);

    RtlInitUnicodeString( &nameW, ActiveComputerNameW );
    if ( ( st = NtCreateKey( &hsubkey, KEY_ALL_ACCESS, &attr, 0, NULL, REG_OPTION_VOLATILE, NULL ) )
         != STATUS_SUCCESS )
        goto out;
    
    RtlInitUnicodeString( &nameW, ComputerNameW );
    st = NtSetValueKey( hsubkey, &nameW, 0, REG_SZ, computer_name, len );

out:
    NtClose( hsubkey );
    NtClose( hkey );

    if ( st == STATUS_SUCCESS )
        TRACE( "success\n" );
    else
    {
        WARN( "status trying to set ComputerName: %x\n", st );
        SetLastError ( RtlNtStatusToDosError ( st ) );
    }
}


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
