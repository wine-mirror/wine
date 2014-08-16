/*
 * Copyright (c) 2006 Robert Reif
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
 *
 */

#ifndef __WINE_ADVAPI32MISC_H
#define __WINE_ADVAPI32MISC_H

#include "ntsecapi.h"
#include "winsvc.h"
#include "winnls.h"

const char * debugstr_sid(PSID sid) DECLSPEC_HIDDEN;
BOOL ADVAPI_IsLocalComputer(LPCWSTR ServerName) DECLSPEC_HIDDEN;
BOOL ADVAPI_GetComputerSid(PSID sid) DECLSPEC_HIDDEN;

BOOL lookup_local_wellknown_name(const LSA_UNICODE_STRING*, PSID, LPDWORD, LPWSTR, LPDWORD, PSID_NAME_USE, BOOL*) DECLSPEC_HIDDEN;
BOOL lookup_local_user_name(const LSA_UNICODE_STRING*, PSID, LPDWORD, LPWSTR, LPDWORD, PSID_NAME_USE, BOOL*) DECLSPEC_HIDDEN;
WCHAR *SERV_dup(const char *str) DECLSPEC_HIDDEN;
DWORD SERV_OpenSCManagerW(LPCWSTR, LPCWSTR, DWORD, SC_HANDLE*) DECLSPEC_HIDDEN;
DWORD SERV_OpenServiceW(SC_HANDLE, LPCWSTR, DWORD, SC_HANDLE*) DECLSPEC_HIDDEN;
NTSTATUS SERV_QueryServiceObjectSecurity(SC_HANDLE, SECURITY_INFORMATION, PSECURITY_DESCRIPTOR, DWORD, LPDWORD) DECLSPEC_HIDDEN;

/* heap allocation helpers */
static void *heap_alloc( size_t len ) __WINE_ALLOC_SIZE(1);
static inline void *heap_alloc( size_t len )
{
    return HeapAlloc( GetProcessHeap(), 0, len );
}

static void *heap_alloc_zero( size_t len ) __WINE_ALLOC_SIZE(1);
static inline void *heap_alloc_zero( size_t len )
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len );
}

static void *heap_realloc( void *mem, size_t len ) __WINE_ALLOC_SIZE(2);
static inline void *heap_realloc( void *mem, size_t len )
{
    return HeapReAlloc( GetProcessHeap(), 0, mem, len );
}

static inline BOOL heap_free( void *mem )
{
    return HeapFree( GetProcessHeap(), 0, mem );
}

static inline WCHAR *strdupAW( const char *src )
{
    WCHAR *dst = NULL;
    if (src)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, src, -1, NULL, 0 );
        if ((dst = heap_alloc( len * sizeof(WCHAR) ))) MultiByteToWideChar( CP_ACP, 0, src, -1, dst, len );
    }
    return dst;
}

#endif /* __WINE_ADVAPI32MISC_H */
