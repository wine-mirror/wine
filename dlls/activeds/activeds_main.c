/*
 * Implementation of the Active Directory Service Interface
 *
 * Copyright 2005 Detlef Riekenberg
 * Copyright 2019 Dmitry Timoshkov
 *
 * This file contains only stubs to get the printui.dll up and running
 * activeds.dll is much much more than this
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

#include "objbase.h"
#include "initguid.h"
#include "iads.h"
#include "adshlp.h"
#include "adserr.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(activeds);

/*****************************************************
 * DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %d, %p)\n",hinstDLL, fdwReason, lpvReserved);

    switch(fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinstDLL );
        break;
    }
    return TRUE;
}

/*****************************************************
 * ADsGetObject     [ACTIVEDS.3]
 */
HRESULT WINAPI ADsGetObject(LPCWSTR lpszPathName, REFIID riid, VOID** ppObject)
{
    FIXME("(%s)->(%s,%p)!stub\n",debugstr_w(lpszPathName), debugstr_guid(riid), ppObject);
    return E_NOTIMPL;
}

/*****************************************************
 * ADsBuildEnumerator    [ACTIVEDS.4]
 */
HRESULT WINAPI ADsBuildEnumerator(IADsContainer * pADsContainer, IEnumVARIANT** ppEnumVariant)
{
    FIXME("(%p)->(%p)!stub\n",pADsContainer, ppEnumVariant);
    return E_NOTIMPL;
}

/*****************************************************
 * ADsFreeEnumerator     [ACTIVEDS.5]
 */
HRESULT WINAPI ADsFreeEnumerator(IEnumVARIANT* pEnumVariant)
{
    FIXME("(%p)!stub\n",pEnumVariant);
    return E_NOTIMPL;
}

/*****************************************************
 * ADsEnumerateNext     [ACTIVEDS.6]
 */
HRESULT WINAPI ADsEnumerateNext(IEnumVARIANT* pEnumVariant, ULONG cElements, VARIANT* pvar, ULONG * pcElementsFetched)
{
    FIXME("(%p)->(%u, %p, %p)!stub\n",pEnumVariant, cElements, pvar, pcElementsFetched);
    return E_NOTIMPL;
}

/*****************************************************
 * ADsBuildVarArrayStr     [ACTIVEDS.7]
 */
HRESULT WINAPI ADsBuildVarArrayStr(LPWSTR *lppPathNames, DWORD dwPathNames, VARIANT* pvar)
{
    FIXME("(%p, %d, %p)!stub\n",*lppPathNames, dwPathNames, pvar);
    return E_NOTIMPL;
}

/*****************************************************
 * ADsBuildVarArrayInt     [ACTIVEDS.8]
 */
HRESULT WINAPI ADsBuildVarArrayInt(LPDWORD lpdwObjectTypes, DWORD dwObjectTypes, VARIANT* pvar)
{
    FIXME("(%p, %d, %p)!stub\n",lpdwObjectTypes, dwObjectTypes, pvar);
    return E_NOTIMPL;
}

/*****************************************************
 * ADsOpenObject     [ACTIVEDS.9]
 */
HRESULT WINAPI ADsOpenObject(LPCWSTR path, LPCWSTR user, LPCWSTR password, DWORD reserved, REFIID riid, void **obj)
{
    HRESULT hr;
    HKEY hkey, hprov;
    WCHAR provider[MAX_PATH], progid[MAX_PATH];
    DWORD idx = 0;

    TRACE("(%s,%s,%u,%s,%p)\n", debugstr_w(path), debugstr_w(user), reserved, debugstr_guid(riid), obj);

    if (!path || !riid || !obj)
        return E_INVALIDARG;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\ADs\\Providers", 0, KEY_READ, &hkey))
        return E_ADS_BAD_PATHNAME;

    hr = E_ADS_BAD_PATHNAME;

    for (;;)
    {
        if (RegEnumKeyW(hkey, idx++, provider, ARRAY_SIZE(provider)))
            break;

        TRACE("provider %s\n", debugstr_w(provider));

        if (!wcsnicmp(path, provider, wcslen(provider)) && path[wcslen(provider)] == ':')
        {
            LONG size;

            if (RegOpenKeyExW(hkey, provider, 0, KEY_READ, &hprov))
                break;

            size = ARRAY_SIZE(progid);
            if (!RegQueryValueW(hprov, NULL, progid, &size))
            {
                CLSID clsid;

                if (CLSIDFromProgID(progid, &clsid) == S_OK)
                {
                    IADsOpenDSObject *adsopen;
                    IDispatch *disp;

                    TRACE("ns %s => clsid %s\n", debugstr_w(progid), wine_dbgstr_guid(&clsid));
                    if (CoCreateInstance(&clsid, 0, CLSCTX_INPROC_SERVER, &IID_IADsOpenDSObject, (void **)&adsopen) == S_OK)
                    {
                        BSTR bpath, buser, bpassword;

                        bpath = SysAllocString(path);
                        buser = SysAllocString(user);
                        bpassword = SysAllocString(password);

                        hr = IADsOpenDSObject_OpenDSObject(adsopen, bpath, buser, bpassword, reserved, &disp);
                        if (hr == S_OK)
                        {
                            hr = IDispatch_QueryInterface(disp, riid, obj);
                            IDispatch_Release(disp);
                        }

                        SysFreeString(bpath);
                        SysFreeString(buser);
                        SysFreeString(bpassword);

                        IADsOpenDSObject_Release(adsopen);
                    }
                }
            }

            RegCloseKey(hprov);
            break;
        }
    }

    RegCloseKey(hkey);

    return hr;
}

/*****************************************************
 * ADsSetLastError    [ACTIVEDS.12]
 */
VOID WINAPI ADsSetLastError(DWORD dwErr, LPWSTR pszError, LPWSTR pszProvider)
{
    FIXME("(%d,%p,%p)!stub\n", dwErr, pszError, pszProvider);
}

/*****************************************************
 * ADsGetLastError    [ACTIVEDS.13]
 */
HRESULT WINAPI ADsGetLastError(LPDWORD perror, LPWSTR errorbuf, DWORD errorbuflen, LPWSTR namebuf, DWORD namebuflen)
{
    FIXME("(%p,%p,%d,%p,%d)!stub\n", perror, errorbuf, errorbuflen, namebuf, namebuflen);
    return E_NOTIMPL;
}

/*****************************************************
 * AllocADsMem             [ACTIVEDS.14]
 */
LPVOID WINAPI AllocADsMem(DWORD cb)
{
    FIXME("(%d)!stub\n",cb);
    return NULL;
}

/*****************************************************
 * FreeADsMem             [ACTIVEDS.15]
 */
BOOL WINAPI FreeADsMem(LPVOID pMem)
{
    FIXME("(%p)!stub\n",pMem);
    return FALSE;
}

/*****************************************************
 * ReallocADsMem             [ACTIVEDS.16]
 */
LPVOID WINAPI ReallocADsMem(LPVOID pOldMem, DWORD cbOld, DWORD cbNew)
{
    FIXME("(%p,%d,%d)!stub\n", pOldMem, cbOld, cbNew);
    return NULL;
}

/*****************************************************
 * AllocADsStr             [ACTIVEDS.17]
 */
LPWSTR WINAPI AllocADsStr(LPWSTR pStr)
{
    FIXME("(%p)!stub\n",pStr);
    return NULL;
}

/*****************************************************
 * FreeADsStr             [ACTIVEDS.18]
 */
BOOL WINAPI FreeADsStr(LPWSTR pStr)
{
    FIXME("(%p)!stub\n",pStr);
    return FALSE;
}

/*****************************************************
 * ReallocADsStr             [ACTIVEDS.19]
 */
BOOL WINAPI ReallocADsStr(LPWSTR *ppStr, LPWSTR pStr)
{
    FIXME("(%p,%p)!stub\n",*ppStr, pStr);
    return FALSE;
}

/*****************************************************
 * ADsEncodeBinaryData     [ACTIVEDS.20]
 */
HRESULT WINAPI ADsEncodeBinaryData(PBYTE pbSrcData, DWORD dwSrcLen, LPWSTR *ppszDestData)
{
    FIXME("(%p,%d,%p)!stub\n", pbSrcData, dwSrcLen, *ppszDestData);
    return E_NOTIMPL;
}
