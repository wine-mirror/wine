/*
 * Implementation of the Fusion API
 *
 * Copyright 2008 James Hawkins
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "fusion.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(fusion);

/******************************************************************
 *  ClearDownloadCache   (FUSION.@)
 */
HRESULT WINAPI ClearDownloadCache(void)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

/******************************************************************
 *  CompareAssemblyIdentity   (FUSION.@)
 */
HRESULT WINAPI CompareAssemblyIdentity(LPCWSTR pwzAssemblyIdentity1, BOOL fUnified1,
                                       LPCWSTR pwzAssemblyIdentity2, BOOL fUnified2,
                                       BOOL *pfEquivalent, AssemblyComparisonResult *pResult)
{
    FIXME("(%s, %d, %s, %d, %p, %p) stub!\n", debugstr_w(pwzAssemblyIdentity1),
          fUnified1, debugstr_w(pwzAssemblyIdentity2), fUnified2, pfEquivalent, pResult);

    return E_NOTIMPL;
}

/******************************************************************
 *  CreateAssemblyEnum   (FUSION.@)
 */
HRESULT WINAPI CreateAssemblyEnum(IAssemblyEnum **pEnum, IUnknown *pUnkReserved,
                                  IAssemblyName *pName, DWORD dwFlags, LPVOID pvReserved)
{
    FIXME("(%p, %p, %p, %08x, %p) stub!\n", pEnum, pUnkReserved,
          pName, dwFlags, pvReserved);

    return E_NOTIMPL;
}

/******************************************************************
 *  CreateAssemblyNameObject   (FUSION.@)
 */
HRESULT WINAPI CreateAssemblyNameObject(LPASSEMBLYNAME *ppAssemblyNameObj,
                                        LPCWSTR szAssemblyName, DWORD dwFlags,
                                        LPVOID pvReserved)
{
    FIXME("(%p, %s, %08x, %p) stub!\n", ppAssemblyNameObj,
          debugstr_w(szAssemblyName), dwFlags, pvReserved);

    return E_NOTIMPL;
}

/******************************************************************
 *  CreateInstallReferenceEnum   (FUSION.@)
 */
HRESULT WINAPI CreateInstallReferenceEnum(IInstallReferenceEnum **ppRefEnum,
                                          IAssemblyName *pName, DWORD dwFlags,
                                          LPVOID pvReserved)
{
    FIXME("(%p, %p, %08x, %p) stub!\n", ppRefEnum, pName, dwFlags, pvReserved);
    return E_NOTIMPL;
}

/******************************************************************
 *  GetAssemblyIdentityFromFile   (FUSION.@)
 */
HRESULT WINAPI GetAssemblyIdentityFromFile(LPCWSTR pwzFilePath, REFIID riid,
                                           IUnknown **ppIdentity)
{
    FIXME("(%s, %s, %p) stub!\n", debugstr_w(pwzFilePath), debugstr_guid(riid),
          ppIdentity);

    return E_NOTIMPL;
}

/******************************************************************
 *  GetCachePath   (FUSION.@)
 */
HRESULT WINAPI GetCachePath(ASM_CACHE_FLAGS dwCacheFlags, LPWSTR pwzCachePath,
                            PDWORD pcchPath)
{
    FIXME("(%08x, %s, %p) stub!\n", dwCacheFlags, debugstr_w(pwzCachePath), pcchPath);
    return E_NOTIMPL;
}
