/*
 * Wininet - Url Cache functions
 *
 * Copyright 2001,2002 CodeWeavers
 *
 * Eric Kohl
 * Aric Stewart
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winerror.h"

#include "wine/debug.h"
#include "internet.h"

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

INTERNETAPI GROUPID WINAPI CreateUrlCacheGroup(DWORD dwFlags, LPVOID 
lpReserved)
{
  FIXME("stub\n");
  return FALSE;
}

INTERNETAPI HANDLE WINAPI FindFirstUrlCacheEntryA(LPCSTR lpszUrlSearchPattern,
 LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo, LPDWORD lpdwFirstCacheEntryInfoBufferSize)
{
  FIXME("STUB\n");
  return 0;
}

INTERNETAPI HANDLE WINAPI FindFirstUrlCacheEntryW(LPCWSTR lpszUrlSearchPattern,
 LPINTERNET_CACHE_ENTRY_INFOW lpFirstCacheEntryInfo, LPDWORD lpdwFirstCacheEntryInfoBufferSize)
{
  FIXME("STUB\n");
  return 0;
}

BOOL WINAPI RetrieveUrlCacheEntryFileA (LPCSTR lpszUrlName,
                                        LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo, LPDWORD
                                        lpdwCacheEntryInfoBufferSize, DWORD dwReserved)
{
    FIXME("STUB\n");
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

BOOL WINAPI DeleteUrlCacheEntry(LPCSTR lpszUrlName)
{
    FIXME("STUB (%s)\n",lpszUrlName);
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

BOOL WINAPI DeleteUrlCacheGroup(GROUPID GroupId, DWORD dwFlags, LPVOID lpReserved)
{
    FIXME("STUB\n");
    return FALSE;
}

BOOL WINAPI SetUrlCacheEntryGroup(LPCSTR lpszUrlName, DWORD dwFlags,
  GROUPID GroupId, LPBYTE pbGroupAttributes, DWORD cbGroupAttributes,
  LPVOID lpReserved)
{
    FIXME("STUB\n");
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

/***********************************************************************
 *           CommitUrlCacheEntryA (WININET.@)
 *
 */
BOOL WINAPI CommitUrlCacheEntryA(LPCSTR lpszUrl, LPCSTR lpszLocalName,
    FILETIME ExpireTime, FILETIME lastModified, DWORD cacheEntryType,
    LPBYTE lpHeaderInfo, DWORD headerSize, LPCSTR fileExtension,
    DWORD originalUrl)
{
    FIXME("stub\n");
    return FALSE;
}

/***********************************************************************
 *           GetUrlCacheEntryInfoA (WININET.@)
 *
 */
BOOL WINAPI GetUrlCacheEntryInfoA(LPCSTR lpszUrl,
  LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntry,
  LPDWORD lpCacheEntrySize)
{
    FIXME("(%s) stub\n",lpszUrl);
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

/***********************************************************************
 *           GetUrlCacheEntryInfoExA (WININET.@)
 *
 */
BOOL WINAPI GetUrlCacheEntryInfoExA(
    LPCSTR lpszUrl,
    LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    LPDWORD lpdwCacheEntryInfoBufSize,
    LPSTR lpszReserved,
    LPDWORD lpdwReserved,
    LPVOID lpReserved,
    DWORD dwFlags)
{
    FIXME(" url=%s, flags=%ld\n",lpszUrl,dwFlags);
    INTERNET_SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

/***********************************************************************
 *           GetUrlCacheEntryInfoExW (WININET.@)
 *
 */
BOOL WINAPI GetUrlCacheEntryInfoExW(
    LPCWSTR lpszUrl,
    LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    LPDWORD lpdwCacheEntryInfoBufSize,
    LPWSTR lpszReserved,
    LPDWORD lpdwReserved,
    LPVOID lpReserved,
    DWORD dwFlags)
{
    FIXME(" url=%s, flags=%ld\n",debugstr_w(lpszUrl),dwFlags);
    INTERNET_SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}
