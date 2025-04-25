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
 *  InitializeFusion   (FUSION.@)
 */
HRESULT WINAPI InitializeFusion(void)
{
    FIXME("\n");
    return S_OK;
}

/******************************************************************
 *  ClearDownloadCache   (FUSION.@)
 */
HRESULT WINAPI ClearDownloadCache(void)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

/******************************************************************
 *  CopyPDBs   (FUSION.@)
 */
HRESULT WINAPI CopyPDBs(void *unknown)
{
    FIXME("(%p) stub!\n", unknown);
    return E_NOTIMPL;
}

/******************************************************************
 *  CreateInstallReferenceEnum   (FUSION.@)
 */
HRESULT WINAPI CreateInstallReferenceEnum(IInstallReferenceEnum **ppRefEnum,
                                          IAssemblyName *pName, DWORD dwFlags,
                                          LPVOID pvReserved)
{
    FIXME("(%p, %p, %08lx, %p) stub!\n", ppRefEnum, pName, dwFlags, pvReserved);
    return E_NOTIMPL;
}

/******************************************************************
 *  CreateApplicationContext   (FUSION.@)
 */
HRESULT WINAPI CreateApplicationContext(IAssemblyName *name, void *ctx)
{
    FIXME("%p, %p\n", name, ctx);
    return E_NOTIMPL;
}

static HRESULT (WINAPI *pGetCORVersion)(LPWSTR pbuffer, DWORD cchBuffer,
                                        DWORD *dwLength);

static HRESULT get_corversion(LPWSTR version, DWORD size)
{
    HMODULE hmscoree;
    HRESULT hr;
    DWORD len;

    hmscoree = LoadLibraryA("mscoree.dll");
    if (!hmscoree)
        return E_FAIL;

    pGetCORVersion = (void *)GetProcAddress(hmscoree, "GetCORVersion");
    if (!pGetCORVersion)
        hr = E_FAIL;
    else
        hr = pGetCORVersion(version, size, &len);

    FreeLibrary(hmscoree);
    return hr;
}

/******************************************************************
 *  GetCachePath   (FUSION.@)
 */
HRESULT WINAPI GetCachePath(ASM_CACHE_FLAGS dwCacheFlags, LPWSTR pwzCachePath,
                            PDWORD pcchPath)
{
#ifdef _WIN64
    static const WCHAR zapfmt[] = L"%s\\assembly\\NativeImages_%s_64";
#else
    static const WCHAR zapfmt[] = L"%s\\assembly\\NativeImages_%s_32";
#endif
    WCHAR path[MAX_PATH], windir[MAX_PATH], version[MAX_PATH];
    DWORD len;
    HRESULT hr = S_OK;

    TRACE("(%08x, %p, %p)\n", dwCacheFlags, pwzCachePath, pcchPath);

    if (!pcchPath)
        return E_INVALIDARG;

    len = GetWindowsDirectoryW(windir, MAX_PATH);
    lstrcpyW(path, windir);

    switch (dwCacheFlags)
    {
        case ASM_CACHE_ZAP:
        {
            hr = get_corversion(version, MAX_PATH);
            if (FAILED(hr))
                return hr;

            len = swprintf(path, ARRAY_SIZE(path), zapfmt, windir, version);
            break;
        }
        case ASM_CACHE_GAC:
        {
            lstrcpyW(path + len, L"\\assembly");
            len += ARRAY_SIZE(L"\\assembly") - 1;
            lstrcpyW(path + len, L"\\GAC");
            len += ARRAY_SIZE(L"\\GAC") - 1;
            break;
        }
        case ASM_CACHE_DOWNLOAD:
        {
            FIXME("Download cache not implemented\n");
            return E_FAIL;
        }
        case ASM_CACHE_ROOT:
            lstrcpyW(path + len, L"\\assembly");
            len += ARRAY_SIZE(L"\\assembly") - 1;
            break;
        case ASM_CACHE_ROOT_EX:
            lstrcpyW(path + len, L"\\Microsoft.NET");
            len += ARRAY_SIZE(L"\\Microsoft.NET") - 1;
            lstrcpyW(path + len, L"\\assembly");
            len += ARRAY_SIZE(L"\\assembly") - 1;
            break;
        default:
            return E_INVALIDARG;
    }

    len++;
    if (*pcchPath <= len || !pwzCachePath)
        hr = E_NOT_SUFFICIENT_BUFFER;
    else if (pwzCachePath)
        lstrcpyW(pwzCachePath, path);

    *pcchPath = len;
    return hr;
}
