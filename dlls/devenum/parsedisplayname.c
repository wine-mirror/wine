/*
 *	IParseDisplayName implementation for DEVENUM.dll
 *
 * Copyright (C) 2002 Robert Shearman
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
 * NOTES ON THIS FILE:
 * - Implements IParseDisplayName interface which creates a moniker
 *   from a string in a special format
 */
#include "devenum_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(devenum);

static HRESULT WINAPI DEVENUM_IParseDisplayName_QueryInterface(IParseDisplayName *iface,
        REFIID riid, void **ppv)
{
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (!ppv)
        return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IParseDisplayName))
    {
        *ppv = iface;
        IParseDisplayName_AddRef(iface);
        return S_OK;
    }

    FIXME("- no interface IID: %s\n", debugstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI DEVENUM_IParseDisplayName_AddRef(IParseDisplayName *iface)
{
    TRACE("\n");

    DEVENUM_LockModule();

    return 2; /* non-heap based object */
}

static ULONG WINAPI DEVENUM_IParseDisplayName_Release(IParseDisplayName *iface)
{
    TRACE("\n");

    DEVENUM_UnlockModule();

    return 1; /* non-heap based object */
}

/**********************************************************************
 * DEVENUM_IParseDisplayName_ParseDisplayName
 *
 *  Creates a moniker referenced to by the display string argument
 *
 * POSSIBLE BUGS:
 *  Might not handle more complicated strings properly (ie anything
 *  not in "@device:sw:{CLSID1}\<filter name or CLSID>" format
 */
static HRESULT WINAPI DEVENUM_IParseDisplayName_ParseDisplayName(IParseDisplayName *iface,
        IBindCtx *pbc, LPOLESTR pszDisplayName, ULONG *pchEaten, IMoniker **ppmkOut)
{
    LPOLESTR pszBetween = NULL;
    LPOLESTR pszClass = NULL;
    MediaCatMoniker * pMoniker = NULL;
    CLSID clsidDevice;
    HRESULT hr = S_OK;
    WCHAR wszRegKeyName[MAX_PATH];
    enum device_type type;
    HKEY hbasekey;
    int classlen;
    LONG res;
    static const WCHAR wszRegSeparator[] =   {'\\', 0 };

    TRACE("(%p, %s, %p, %p)\n", pbc, debugstr_w(pszDisplayName), pchEaten, ppmkOut);

    *ppmkOut = NULL;
    if (pchEaten)
        *pchEaten = strlenW(pszDisplayName);

    pszDisplayName = strchrW(pszDisplayName, ':') + 1;
    if (pszDisplayName[0] == 's' && pszDisplayName[1] == 'w' && pszDisplayName[2] == ':')
    {
        type = DEVICE_FILTER;
        if ((res = RegOpenKeyExW(HKEY_CLASSES_ROOT, clsidW, 0, 0, &hbasekey)))
            return HRESULT_FROM_WIN32(res);
    }
    else if (pszDisplayName[0] == 'c' && pszDisplayName[1] == 'm' && pszDisplayName[2] == ':')
    {
        type = DEVICE_CODEC;
        if ((res = RegOpenKeyExW(HKEY_CURRENT_USER, wszActiveMovieKey, 0, 0, &hbasekey)))
            return HRESULT_FROM_WIN32(res);
    }
    else
    {
        FIXME("unhandled device type %s\n", debugstr_w(pszDisplayName+1));
        return MK_E_SYNTAX;
    }

    pszDisplayName = strchrW(pszDisplayName, '{');
    pszBetween = strchrW(pszDisplayName, '}') + 2;

    /* size = pszBetween - pszDisplayName - 1 (for '\\' after CLSID)
     * + 1 (for NULL character)
     */
    classlen = (int)(pszBetween - pszDisplayName - 1);
    pszClass = CoTaskMemAlloc((classlen + 1) * sizeof(WCHAR));
    if (!pszClass)
        return E_OUTOFMEMORY;

    memcpy(pszClass, pszDisplayName, classlen * sizeof(WCHAR));
    pszClass[classlen] = 0;

    TRACE("Device CLSID: %s\n", debugstr_w(pszClass));

    hr = CLSIDFromString(pszClass, &clsidDevice);

    if (SUCCEEDED(hr))
    {
        pMoniker = DEVENUM_IMediaCatMoniker_Construct();
        if (pMoniker)
        {
            strcpyW(wszRegKeyName, pszClass);
            if (type == DEVICE_FILTER)
                strcatW(wszRegKeyName, instanceW);
            strcatW(wszRegKeyName, wszRegSeparator);
            strcatW(wszRegKeyName, pszBetween);
            if (RegCreateKeyW(hbasekey, wszRegKeyName, &pMoniker->hkey) == ERROR_SUCCESS)
                *ppmkOut = &pMoniker->IMoniker_iface;
            else
            {
                IMoniker_Release(&pMoniker->IMoniker_iface);
                hr = MK_E_NOOBJECT;
            }
        }
    }

    CoTaskMemFree(pszClass);

    TRACE("-- returning: %x\n", hr);
    return hr;
}

/**********************************************************************
 * IParseDisplayName_Vtbl
 */
static const IParseDisplayNameVtbl IParseDisplayName_Vtbl =
{
    DEVENUM_IParseDisplayName_QueryInterface,
    DEVENUM_IParseDisplayName_AddRef,
    DEVENUM_IParseDisplayName_Release,
    DEVENUM_IParseDisplayName_ParseDisplayName
};

/* The one instance of this class */
IParseDisplayName DEVENUM_ParseDisplayName = { &IParseDisplayName_Vtbl };
