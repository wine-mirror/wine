/*
 * WinTrust Cryptography functions
 *
 * Copyright 2006 James Hawkins
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
#include "wintrust.h"
#include "mscat.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintrust);

/***********************************************************************
 *      CryptCATAdminAcquireContext (WINTRUST.@)
 *
 * Get a catalog administrator context handle.
 *
 * PARAMS
 *   catAdmin  [O] Pointer to the context handle.
 *   sysSystem [I] Pointer to a GUID for the needed subsystem.
 *   dwFlags   [I] Reserved.
 *
 * RETURNS
 *   Success: TRUE. catAdmin contains the context handle.
 *   Failure: FAIL.
 *
 */
BOOL WINAPI CryptCATAdminAcquireContext(HCATADMIN* catAdmin,
                    const GUID *sysSystem, DWORD dwFlags )
{
    FIXME("%p %s %lx\n", catAdmin, debugstr_guid(sysSystem), dwFlags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *             CryptCATAdminCalcHashFromFileHandle (WINTRUST.@)
 */
BOOL WINAPI CryptCATAdminCalcHashFromFileHandle(HANDLE hFile, DWORD* pcbHash,
                                                BYTE* pbHash, DWORD dwFlags )
{
    FIXME("%p %p %p %lx\n", hFile, pcbHash, pbHash, dwFlags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *             CryptCATAdminEnumCatalogFromHash (WINTRUST.@)
 */
HCATINFO WINAPI CryptCATAdminEnumCatalogFromHash(HCATADMIN hCatAdmin,
                                                 BYTE* pbHash,
                                                 DWORD cbHash,
                                                 DWORD dwFlags,
                                                 HCATINFO* phPrevCatInfo )
{
    FIXME("%p %p %ld %ld %p\n", hCatAdmin, pbHash, cbHash, dwFlags, phPrevCatInfo);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

/***********************************************************************
 *      CryptCATAdminReleaseContext (WINTRUST.@)
 *
 * Release a catalog administrator context handle.
 *
 * PARAMS
 *   catAdmin  [I] Pointer to the context handle.
 *   dwFlags   [I] Reserved.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FAIL.
 *
 */
BOOL WINAPI CryptCATAdminReleaseContext(HCATADMIN hCatAdmin, DWORD dwFlags )
{
    FIXME("%p %lx\n", hCatAdmin, dwFlags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *      CryptCATClose  (WINTRUST.@)
 */
BOOL WINAPI CryptCATClose(HANDLE hCatalog)
{
    FIXME("(%p) stub\n", hCatalog);
    return TRUE;
}

/***********************************************************************
 *      CryptCATEnumerateMember  (WINTRUST.@)
 */
CRYPTCATMEMBER *WINAPI CryptCATEnumerateMember(HANDLE hCatalog, CRYPTCATMEMBER* pPrevMember)
{
    FIXME("(%p, %p) stub\n", hCatalog, pPrevMember);
    return NULL;
}

/***********************************************************************
 *      CryptCATOpen  (WINTRUST.@)
 */
HANDLE WINAPI CryptCATOpen(LPWSTR pwszFileName, DWORD fdwOpenFlags, HCRYPTPROV hProv,
                           DWORD dwPublicVersion, DWORD dwEncodingType)
{
    FIXME("(%s, %ld, %ld, %ld, %ld) stub\n", debugstr_w(pwszFileName), fdwOpenFlags,
          hProv, dwPublicVersion, dwEncodingType);
    return 0;
}
