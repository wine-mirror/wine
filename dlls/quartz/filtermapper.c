/*
 * IFilterMapper & IFilterMapper2 Implementations
 *
 * Copyright 2003 Robert Shearman
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"

#include "quartz_private.h"

#define COM_NO_WINDOWS_H
#include "ole2.h"
#include "strmif.h"
#include "wine/obj_property.h"
#include "wine/unicode.h"
#include "uuids.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct FilterMapper2Impl
{
    ICOM_VFIELD(IFilterMapper2);
    ICOM_VTABLE(IFilterMapper) * lpVtblFilterMapper;
    ULONG refCount;
} FilterMapper2Impl;

static struct ICOM_VTABLE(IFilterMapper2) fm2vtbl;
static struct ICOM_VTABLE(IFilterMapper) fmvtbl;

#define _IFilterMapper_Offset ((int)(&(((FilterMapper2Impl*)0)->lpVtblFilterMapper)))
#define ICOM_THIS_From_IFilterMapper(impl, iface) impl* This = (impl*)(((char*)iface)-_IFilterMapper_Offset)

static const WCHAR wszClsidSlash[] = {'C','L','S','I','D','\\',0};
static const WCHAR wszSlashInstance[] = {'\\','I','n','s','t','a','n','c','e','\\',0};

/* CLSID property in media category Moniker */
static const WCHAR wszClsidName[] = {'C','L','S','I','D',0};
/* FriendlyName property in media category Moniker */
static const WCHAR wszFriendlyName[] = {'F','r','i','e','n','d','l','y','N','a','m','e',0};
/* Merit property in media category Moniker (CLSID_ActiveMovieCategories only) */
static const WCHAR wszMeritName[] = {'M','e','r','i','t',0};
/* FilterData property in media category Moniker (not CLSID_ActiveMovieCategories) */
static const WCHAR wszFilterDataName[] = {'F','i','l','t','e','r','D','a','t','a',0};

/* registry format for REGFILTER2 */
struct REG_RF
{
    DWORD dwVersion;
    DWORD dwMerit;
    DWORD dwPins;
    DWORD dwUnused;
};

struct REG_RFP
{
    BYTE signature[4]; /* e.g. "0pi3" */
    DWORD dwFlags;
    DWORD dwInstances;
    DWORD dwMediaTypes;
    DWORD dwMediums;
    DWORD bCategory; /* is there a category clsid? */
    /* optional: dwOffsetCategoryClsid */
};

struct REG_TYPE
{
    BYTE signature[4]; /* e.g. "0ty3" */
    DWORD dwUnused;
    DWORD dwOffsetMajor;
    DWORD dwOffsetMinor;
};

struct MONIKER_MERIT
{
    IMoniker * pMoniker;
    DWORD dwMerit;
};

struct Vector
{
    LPBYTE pData;
    int capacity; /* in bytes */
    int current; /* pointer to next free byte */
};

/* returns the position it was added at */
static int add_data(struct Vector * v, const BYTE * pData, int size)
{
    int index = v->current;
    if (v->current + size > v->capacity)
    {
        LPBYTE pOldData = v->pData;
        v->capacity = (v->capacity + size) * 2;
        v->pData = CoTaskMemAlloc(v->capacity);
        memcpy(v->pData, pOldData, v->current);
        CoTaskMemFree(pOldData);
    }
    memcpy(v->pData + v->current, pData, size);
    v->current += size;
    return index;
}

static int find_data(struct Vector * v, const BYTE * pData, int size)
{
    int index;
    for (index = 0; index < v->current; index++)
        if (!memcmp(v->pData + index, pData, size))
            return index;
    /* not found */
    return -1;
}

static void delete_vector(struct Vector * v)
{
    if (v->pData)
        CoTaskMemFree(v->pData);
    v->current = 0;
    v->capacity = 0;
}

HRESULT FilterMapper2_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    FilterMapper2Impl * pFM2impl;

    TRACE("(%p, %p)\n", pUnkOuter, ppObj);

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    pFM2impl = CoTaskMemAlloc(sizeof(*pFM2impl));
    if (!pFM2impl)
        return E_OUTOFMEMORY;

    pFM2impl->lpVtbl = &fm2vtbl;
    pFM2impl->lpVtblFilterMapper = &fmvtbl;
    pFM2impl->refCount = 1;

    *ppObj = pFM2impl;

    TRACE("-- created at %p\n", pFM2impl);

    return S_OK;
}

/*** IUnknown methods ***/

static HRESULT WINAPI FilterMapper2_QueryInterface(IFilterMapper2 * iface, REFIID riid, LPVOID *ppv)
{
    ICOM_THIS(FilterMapper2Impl, iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IFilterMapper2))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IFilterMapper))
        *ppv = &This->lpVtblFilterMapper;

    if (*ppv != NULL)
    {
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }

    FIXME("No interface for %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI FilterMapper2_AddRef(IFilterMapper2 * iface)
{
    ICOM_THIS(FilterMapper2Impl, iface);

    TRACE("()\n");

    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI FilterMapper2_Release(IFilterMapper2 * iface)
{
    ICOM_THIS(FilterMapper2Impl, iface);

    TRACE("()\n");

    if (InterlockedDecrement(&This->refCount) == 0)
    {
        CoTaskMemFree(This);
        return 0;
    }
    return This->refCount;
}

/*** IFilterMapper2 methods ***/

static HRESULT WINAPI FilterMapper2_CreateCategory(
    IFilterMapper2 * iface,
    REFCLSID clsidCategory,
    DWORD dwCategoryMerit,
    LPCWSTR szDescription)
{
    LPWSTR wClsidAMCat = NULL;
    LPWSTR wClsidCategory = NULL;
    WCHAR wszKeyName[strlenW(wszClsidSlash) + strlenW(wszSlashInstance) + (CHARS_IN_GUID-1) * 2 + 1];
    HKEY hKey = NULL;
    HRESULT hr;

    TRACE("(%s, %lx, %s)\n", debugstr_guid(clsidCategory), dwCategoryMerit, debugstr_w(szDescription));

    hr = StringFromCLSID(&CLSID_ActiveMovieCategories, &wClsidAMCat);

    if (SUCCEEDED(hr))
    {
        hr = StringFromCLSID(clsidCategory, &wClsidCategory);
    }

    if (SUCCEEDED(hr))
    {
        strcpyW(wszKeyName, wszClsidSlash);
        strcatW(wszKeyName, wClsidAMCat);
        strcatW(wszKeyName, wszSlashInstance);
        strcatW(wszKeyName, wClsidCategory);

        hr = HRESULT_FROM_WIN32(RegCreateKeyExW(HKEY_CLASSES_ROOT, wszKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL));
    }

    if (SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, wszFriendlyName, 0, REG_SZ, (LPBYTE)szDescription, strlenW(szDescription) + 1));
    }

    if (SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, wszClsidName, 0, REG_SZ, (LPBYTE)wClsidCategory, strlenW(wClsidCategory) + 1));
    }

    if (SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, wszMeritName, 0, REG_DWORD, (LPBYTE)&dwCategoryMerit, sizeof(dwCategoryMerit)));
    }

    CloseHandle(hKey);

    if (wClsidCategory)
        CoTaskMemFree(wClsidCategory);
    if (wClsidAMCat)
        CoTaskMemFree(wClsidAMCat);

    return hr;
}

static HRESULT WINAPI FilterMapper2_UnregisterFilter(
    IFilterMapper2 * iface,
    const CLSID *pclsidCategory,
    const OLECHAR *szInstance,
    REFCLSID Filter)
{
    WCHAR wszKeyName[MAX_PATH];
    LPWSTR wClsidCategory = NULL;
    LPWSTR wFilter = NULL;
    HRESULT hr;

    TRACE("(%p, %s, %s)\n", pclsidCategory, debugstr_w(szInstance), debugstr_guid(Filter));

    if (!pclsidCategory)
        pclsidCategory = &CLSID_LegacyAmFilterCategory;

    hr = StringFromCLSID(pclsidCategory, &wClsidCategory);

    if (SUCCEEDED(hr))
    {
        strcpyW(wszKeyName, wszClsidSlash);
        strcatW(wszKeyName, wClsidCategory);
        strcatW(wszKeyName, wszSlashInstance);
        if (szInstance)
            strcatW(wszKeyName, szInstance);
        else
        {
            hr = StringFromCLSID(Filter, &wFilter);
            if (SUCCEEDED(hr))
                strcatW(wszKeyName, wFilter);
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegDeleteKeyW(HKEY_CLASSES_ROOT, wszKeyName));
    }

    if (wClsidCategory)
        CoTaskMemFree(wClsidCategory);
    if (wFilter)
        CoTaskMemFree(wFilter);

    return hr;
}

static HRESULT FM2_WriteFriendlyName(IPropertyBag * pPropBag, LPCWSTR szName)
{
    VARIANT var;

    V_VT(&var) = VT_BSTR;
    V_UNION(&var, bstrVal) = (BSTR)szName;

    return IPropertyBag_Write(pPropBag, wszFriendlyName, &var);
}

static HRESULT FM2_WriteClsid(IPropertyBag * pPropBag, REFCLSID clsid)
{
    LPWSTR wszClsid = NULL;
    VARIANT var;
    HRESULT hr;

    hr = StringFromCLSID(clsid, &wszClsid);

    if (SUCCEEDED(hr))
    {
        V_VT(&var) = VT_BSTR;
        V_UNION(&var, bstrVal) = wszClsid;
        hr = IPropertyBag_Write(pPropBag, wszClsidName, &var);
    }
    if (wszClsid)
        CoTaskMemFree(wszClsid);
    return hr;
}

static HRESULT FM2_WriteFilterData(IPropertyBag * pPropBag, const REGFILTER2 * prf2)
{
    VARIANT var;
    int size = sizeof(struct REG_RF);
    int i;
    struct Vector mainStore = {NULL, 0, 0};
    struct Vector clsidStore = {NULL, 0, 0};
    struct REG_RF rrf;
    SAFEARRAY * psa;
    SAFEARRAYBOUND saBound;
    HRESULT hr = S_OK;

    rrf.dwVersion = prf2->dwVersion;
    rrf.dwMerit = prf2->dwMerit;
    rrf.dwPins = prf2->u.s1.cPins2;
    rrf.dwUnused = 0;

    add_data(&mainStore, (LPBYTE)&rrf, sizeof(rrf));

    for (i = 0; i < prf2->u.s1.cPins2; i++)
    {
        size += sizeof(struct REG_RFP);
        if (prf2->u.s1.rgPins2[i].clsPinCategory)
            size += sizeof(DWORD);
        size += prf2->u.s1.rgPins2[i].nMediaTypes * sizeof(struct REG_TYPE);
        size += prf2->u.s1.rgPins2[i].nMediums * sizeof(DWORD);
    }

    for (i = 0; i < prf2->u.s1.cPins2; i++)
    {
        struct REG_RFP rrfp;
        REGFILTERPINS2 rgPin2 = prf2->u.s1.rgPins2[i];
        int j;

        rrfp.signature[0] = '0';
        rrfp.signature[1] = 'p';
        rrfp.signature[2] = 'i';
        rrfp.signature[3] = '3';
        rrfp.signature[0] += i;
        rrfp.dwFlags = rgPin2.dwFlags;
        rrfp.dwInstances = rgPin2.cInstances;
        rrfp.dwMediaTypes = rgPin2.nMediaTypes;
        rrfp.dwMediums = rgPin2.nMediums;
        rrfp.bCategory = rgPin2.clsPinCategory ? 1 : 0;

        add_data(&mainStore, (LPBYTE)&rrfp, sizeof(rrfp));
        if (rrfp.bCategory)
        {
            DWORD index = find_data(&clsidStore, (LPBYTE)rgPin2.clsPinCategory, sizeof(CLSID));
            if (index == -1)
                index = add_data(&clsidStore, (LPBYTE)rgPin2.clsPinCategory, sizeof(CLSID));
            index += size;

            add_data(&mainStore, (LPBYTE)&index, sizeof(index));
        }

        for (j = 0; j < rgPin2.nMediaTypes; j++)
        {
            struct REG_TYPE rt;
            rt.signature[0] = '0';
            rt.signature[1] = 't';
            rt.signature[2] = 'y';
            rt.signature[3] = '3';
            rt.signature[0] += j;

            rt.dwUnused = 0;
            rt.dwOffsetMajor = find_data(&clsidStore, (LPBYTE)rgPin2.lpMediaType[j].clsMajorType, sizeof(CLSID));
            if (rt.dwOffsetMajor == -1)
                rt.dwOffsetMajor = add_data(&clsidStore, (LPBYTE)rgPin2.lpMediaType[j].clsMajorType, sizeof(CLSID));
            rt.dwOffsetMajor += size;
            rt.dwOffsetMinor = find_data(&clsidStore, (LPBYTE)rgPin2.lpMediaType[j].clsMinorType, sizeof(CLSID));
            if (rt.dwOffsetMinor == -1)
                rt.dwOffsetMinor = add_data(&clsidStore, (LPBYTE)rgPin2.lpMediaType[j].clsMinorType, sizeof(CLSID));
            rt.dwOffsetMinor += size;

            add_data(&mainStore, (LPBYTE)&rt, sizeof(rt));
        }

        for (j = 0; j < rgPin2.nMediums; j++)
        {
            DWORD index = find_data(&clsidStore, (LPBYTE)(rgPin2.lpMedium + j), sizeof(REGPINMEDIUM));
            if (index == -1)
                index = add_data(&clsidStore, (LPBYTE)(rgPin2.lpMedium + j), sizeof(REGPINMEDIUM));
            index += size;

            add_data(&mainStore, (LPBYTE)&index, sizeof(index));
        }
    }

    saBound.lLbound = 0;
    saBound.cElements = mainStore.current + clsidStore.current;
    psa = SafeArrayCreate(VT_UI1, 1, &saBound);
    if (!psa)
    {
        ERR("Couldn't create SAFEARRAY\n");
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        LPBYTE pbSAData;
        hr = SafeArrayAccessData(psa, (LPVOID *)&pbSAData);
        if (SUCCEEDED(hr))
        {
            memcpy(pbSAData, mainStore.pData, mainStore.current);
            memcpy(pbSAData + mainStore.current, clsidStore.pData, clsidStore.current);
            hr = SafeArrayUnaccessData(psa);
        }
    }

    V_VT(&var) = VT_ARRAY | VT_UI1;
    V_UNION(&var, parray) = psa;

    if (SUCCEEDED(hr))
        hr = IPropertyBag_Write(pPropBag, wszFilterDataName, &var);

    if (psa)
        SafeArrayDestroy(psa);

    delete_vector(&mainStore);
    delete_vector(&clsidStore);
    return hr;
}

static HRESULT FM2_ReadFilterData(IPropertyBag * pPropBag, REGFILTER2 * prf2)
{
    VARIANT var;
    HRESULT hr;
    LPBYTE pData = NULL;
    struct REG_RF * prrf;
    LPBYTE pCurrent;
    DWORD i;
    REGFILTERPINS2 * rgPins2;

    VariantInit(&var);
    V_VT(&var) = VT_ARRAY | VT_UI1;

    hr = IPropertyBag_Read(pPropBag, wszFilterDataName, &var, NULL);

    if (SUCCEEDED(hr))
        hr = SafeArrayAccessData(V_UNION(&var, parray), (LPVOID*)&pData);

    if (SUCCEEDED(hr))
    {
        prrf = (struct REG_RF *)pData;
        pCurrent = pData;

        if (prrf->dwVersion != 2)
        {
            FIXME("Filter registry version %ld not supported\n", prrf->dwVersion);
            ZeroMemory(prf2, sizeof(*prf2));
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        TRACE("version = %ld, merit = %lx, #pins = %ld, unused = %lx\n",
            prrf->dwVersion, prrf->dwMerit, prrf->dwPins, prrf->dwUnused);

        prf2->dwVersion = prrf->dwVersion;
        prf2->dwMerit = prrf->dwMerit;
        prf2->u.s1.cPins2 = prrf->dwPins;
        rgPins2 = CoTaskMemAlloc(prrf->dwPins * sizeof(*rgPins2));
        prf2->u.s1.rgPins2 = rgPins2;
        pCurrent += sizeof(struct REG_RF);

        for (i = 0; i < prrf->dwPins; i++)
        {
            struct REG_RFP * prrfp = (struct REG_RFP *)pCurrent;
            REGPINTYPES * lpMediaType;
            REGPINMEDIUM * lpMedium;
            UINT j;

            /* FIXME: check signature */

            TRACE("\tsignature = %s\n", debugstr_an(prrfp->signature, 4));

            TRACE("\tpin[%ld]: flags = %lx, instances = %ld, media types = %ld, mediums = %ld\n",
                i, prrfp->dwFlags, prrfp->dwInstances, prrfp->dwMediaTypes, prrfp->dwMediums);

            rgPins2[i].dwFlags = prrfp->dwFlags;
            rgPins2[i].cInstances = prrfp->dwInstances;
            rgPins2[i].nMediaTypes = prrfp->dwMediaTypes;
            rgPins2[i].nMediums = prrfp->dwMediums;
            pCurrent += sizeof(struct REG_RFP);
            if (prrfp->bCategory)
            {
                CLSID * clsCat = CoTaskMemAlloc(sizeof(CLSID));
                memcpy(clsCat, pData + *(DWORD*)(pCurrent), sizeof(CLSID));
                pCurrent += sizeof(DWORD);
                rgPins2[i].clsPinCategory = clsCat;
            }
            else
                rgPins2[i].clsPinCategory = NULL;

            if (rgPins2[i].nMediaTypes > 0)
                lpMediaType = CoTaskMemAlloc(rgPins2[i].nMediaTypes * sizeof(*lpMediaType));
            else
                lpMediaType = NULL;

            rgPins2[i].lpMediaType = lpMediaType;

            for (j = 0; j < rgPins2[i].nMediaTypes; j++)
            {
                struct REG_TYPE * prt = (struct REG_TYPE *)pCurrent;
                CLSID * clsMajor = CoTaskMemAlloc(sizeof(CLSID));
                CLSID * clsMinor = CoTaskMemAlloc(sizeof(CLSID));

                /* FIXME: check signature */
                TRACE("\t\tsignature = %s\n", debugstr_an(prt->signature, 4));

                memcpy(clsMajor, pData + prt->dwOffsetMajor, sizeof(CLSID));
                memcpy(clsMinor, pData + prt->dwOffsetMinor, sizeof(CLSID));

                lpMediaType[j].clsMajorType = clsMajor;
                lpMediaType[j].clsMinorType = clsMinor;

                pCurrent += sizeof(*prt);
            }

            if (rgPins2[i].nMediums > 0)
                lpMedium = CoTaskMemAlloc(rgPins2[i].nMediums * sizeof(*lpMedium));
            else
                lpMedium = NULL;

            rgPins2[i].lpMedium = lpMedium;

            for (j = 0; j < rgPins2[i].nMediums; j++)
            {
                DWORD dwOffset = *(DWORD *)pCurrent;

                memcpy(lpMedium + j, pData + dwOffset, sizeof(REGPINMEDIUM));

                pCurrent += sizeof(dwOffset);
            }
        }

    }

    if (pData)
        SafeArrayUnaccessData(V_UNION(&var, parray));

    VariantClear(&var);

    return hr;
}

static void FM2_DeleteRegFilter(REGFILTER2 * prf2)
{
    UINT i;
    for (i = 0; i < prf2->u.s1.cPins2; i++)
    {
        UINT j;
        if (prf2->u.s1.rgPins2[i].clsPinCategory)
            CoTaskMemFree((LPVOID)prf2->u.s1.rgPins2[i].clsPinCategory);

        for (j = 0; j < prf2->u.s1.rgPins2[i].nMediaTypes; j++)
        {
            CoTaskMemFree((LPVOID)prf2->u.s1.rgPins2[i].lpMediaType[j].clsMajorType);
            CoTaskMemFree((LPVOID)prf2->u.s1.rgPins2[i].lpMediaType[j].clsMinorType);
        }
        CoTaskMemFree((LPVOID)prf2->u.s1.rgPins2[i].lpMedium);
    }
}

static HRESULT WINAPI FilterMapper2_RegisterFilter(
    IFilterMapper2 * iface,
    REFCLSID clsidFilter,
    LPCWSTR szName,
    IMoniker **ppMoniker,
    const CLSID *pclsidCategory,
    const OLECHAR *szInstance,
    const REGFILTER2 *prf2)
{
    IParseDisplayName * pParser = NULL;
    IBindCtx * pBindCtx = NULL;
    IMoniker * pMoniker = NULL;
    IPropertyBag * pPropBag = NULL;
    HRESULT hr = S_OK;
    LPWSTR pwszParseName = NULL;
    LPWSTR pCurrent;
    static const WCHAR wszDevice[] = {'@','d','e','v','i','c','e',':','s','w',':',0};
    int nameLen;
    ULONG ulEaten;
    LPWSTR szClsidTemp = NULL;

    TRACE("(%s, %s, %p, %s, %s, %p)\n",
        debugstr_guid(clsidFilter),
        debugstr_w(szName),
        ppMoniker,
        debugstr_guid(pclsidCategory),
        debugstr_w(szInstance),
        prf2);

    if (ppMoniker)
        FIXME("ppMoniker != NULL not supported at the moment\n");

    if (prf2->dwVersion != 2)
    {
        FIXME("dwVersion != 2 not supported at the moment\n");
        return E_NOTIMPL;
    }

    if (!pclsidCategory)
        pclsidCategory = &CLSID_ActiveMovieCategories;

    /* sizeof... will include null terminator and
     * the + 1 is for the separator ('\\'). The -1 is
     * because CHARS_IN_GUID includes the null terminator
     */
    nameLen = sizeof(wszDevice)/sizeof(wszDevice[0]) + CHARS_IN_GUID - 1 + 1;

    if (szInstance)
        nameLen += strlenW(szInstance);
    else
        nameLen += CHARS_IN_GUID - 1; /* CHARS_IN_GUID includes null terminator */

    pwszParseName = CoTaskMemAlloc(nameLen);
    pCurrent = pwszParseName;
    if (!pwszParseName)
        return E_OUTOFMEMORY;

    strcpyW(pwszParseName, wszDevice);
    pCurrent += strlenW(wszDevice);

    hr = StringFromCLSID(pclsidCategory, &szClsidTemp);
    strcpyW(pCurrent, szClsidTemp);
    pCurrent += CHARS_IN_GUID - 1;
    pCurrent[0] = '\\';

    if (SUCCEEDED(hr))
    {
        if (szInstance)
            strcpyW(pCurrent+1, szInstance);
        else
        {
            if (szClsidTemp)
            {
                CoTaskMemFree(szClsidTemp);
                szClsidTemp = NULL;
            }
            hr = StringFromCLSID(clsidFilter, &szClsidTemp);
            strcpyW(pCurrent+1, szClsidTemp);
        }
    }

    if (SUCCEEDED(hr))
        hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (LPVOID *)&pParser);

    if (SUCCEEDED(hr))
        hr = CreateBindCtx(0, &pBindCtx);

    if (SUCCEEDED(hr))
    {
        hr = IParseDisplayName_ParseDisplayName(pParser, pBindCtx, pwszParseName, &ulEaten, &pMoniker);
    }

    if (pBindCtx)
        IBindCtx_Release(pBindCtx); pBindCtx = NULL;

    if (pParser)
        IParseDisplayName_Release(pParser); pParser = NULL;

    if (SUCCEEDED(hr))
        hr = IMoniker_BindToStorage(pMoniker, NULL, NULL, &IID_IPropertyBag, (LPVOID)&pPropBag);

    if (SUCCEEDED(hr))
        hr = FM2_WriteFriendlyName(pPropBag, szName);

    if (SUCCEEDED(hr))
        hr = FM2_WriteClsid(pPropBag, clsidFilter);

    if (SUCCEEDED(hr))
        hr = FM2_WriteFilterData(pPropBag, prf2);

    if (pMoniker)
        IMoniker_Release(pMoniker); pMoniker = NULL;

    if (pPropBag)
        IPropertyBag_Release(pPropBag); pPropBag = NULL;

    if (szClsidTemp)
        CoTaskMemFree(szClsidTemp);

    TRACE("-- returning %lx\n", hr);

    return hr;
}

/* internal helper function */
static BOOL MatchTypes(
    BOOL bExactMatch,
    DWORD nPinTypes,
    const REGPINTYPES * pPinTypes,
    DWORD nMatchTypes,
    const GUID * pMatchTypes)
{
    BOOL bMatch = FALSE;
    DWORD j;

    if ((nMatchTypes == 0) && (nPinTypes > 0))
        bMatch = TRUE;

    for (j = 0; j < nPinTypes; j++)
    {
        DWORD i;
        for (i = 0; i < nMatchTypes*2; i+=2)
        {
            if (((!bExactMatch && IsEqualGUID(pPinTypes[j].clsMajorType, &GUID_NULL)) || IsEqualGUID(&pMatchTypes[i], &GUID_NULL) || IsEqualGUID(pPinTypes[j].clsMajorType, &pMatchTypes[i])) &&
                ((!bExactMatch && IsEqualGUID(pPinTypes[j].clsMinorType, &GUID_NULL)) || IsEqualGUID(&pMatchTypes[i+1], &GUID_NULL) || IsEqualGUID(pPinTypes[j].clsMinorType, &pMatchTypes[i+1])))
            {
                bMatch = TRUE;
                break;
            }
        }
    }
    return bMatch;
}

/* internal helper function for qsort of MONIKER_MERIT array */
static int mm_compare(const void * left, const void * right)
{
    const struct MONIKER_MERIT * mmLeft = (struct MONIKER_MERIT *)left;
    const struct MONIKER_MERIT * mmRight = (struct MONIKER_MERIT *)right;

    if (mmLeft->dwMerit == mmRight->dwMerit)
        return 0;
    if (mmLeft->dwMerit > mmRight->dwMerit)
        return -1;
    return 1;
}

/* NOTES:
 *   Exact match means whether or not to treat GUID_NULL's in filter data as wild cards
 *    (GUID_NULL's in input to function automatically treated as wild cards)
 *   Input/Output needed means match only on criteria if TRUE (with zero input types
 *    meaning match any input/output pin as long as one exists), otherwise match any
 *    filter that meets the rest of the requirements.
 */
static HRESULT WINAPI FilterMapper2_EnumMatchingFilters(
    IFilterMapper2 * iface,
    IEnumMoniker **ppEnum,
    DWORD dwFlags,
    BOOL bExactMatch,
    DWORD dwMerit,
    BOOL bInputNeeded,
    DWORD cInputTypes,
    const GUID *pInputTypes,
    const REGPINMEDIUM *pMedIn,
    const CLSID *pPinCategoryIn,
    BOOL bRender,
    BOOL bOutputNeeded,
    DWORD cOutputTypes,
    const GUID *pOutputTypes,
    const REGPINMEDIUM *pMedOut,
    const CLSID *pPinCategoryOut)
{
    ICreateDevEnum * pCreateDevEnum;
    IMoniker * pMonikerCat;
    IEnumMoniker * pEnumCat;
    HRESULT hr;
    struct Vector monikers = {NULL, 0, 0};

    TRACE("(%p, %lx, %s, %lx, %s, %ld, %p, %p, %p, %s, %s, %p, %p, %p)\n",
        ppEnum,
        dwFlags,
        bExactMatch ? "true" : "false",
        dwMerit,
        bInputNeeded ? "true" : "false",
        cInputTypes,
        pInputTypes,
        pMedIn,
        pPinCategoryIn,
        bRender ? "true" : "false",
        bOutputNeeded ? "true" : "false",
        pOutputTypes,
        pMedOut,
        pPinCategoryOut);

    if (dwFlags != 0)
    {
        FIXME("dwFlags = %lx not implemented\n", dwFlags);
    }

    *ppEnum = NULL;

    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, &IID_ICreateDevEnum, (LPVOID*)&pCreateDevEnum);

    if (SUCCEEDED(hr))
        hr = ICreateDevEnum_CreateClassEnumerator(pCreateDevEnum, &CLSID_ActiveMovieCategories, &pEnumCat, 0);

    while (IEnumMoniker_Next(pEnumCat, 1, &pMonikerCat, NULL) == S_OK)
    {
        IPropertyBag * pPropBagCat = NULL;
        VARIANT var;
        HRESULT hrSub; /* this is so that one buggy filter
                          doesn't make the whole lot fail */

        VariantInit(&var);

        hrSub = IMoniker_BindToStorage(pMonikerCat, NULL, NULL, &IID_IPropertyBag, (LPVOID*)&pPropBagCat);

        if (SUCCEEDED(hrSub))
            hrSub = IPropertyBag_Read(pPropBagCat, wszMeritName, &var, NULL);

        if (SUCCEEDED(hrSub) && (V_UNION(&var, ulVal) >= dwMerit))
        {
            CLSID clsidCat;
            IEnumMoniker * pEnum;
            IMoniker * pMoniker;

            VariantClear(&var);

            if (TRACE_ON(quartz))
            {
                VARIANT temp;
                V_VT(&temp) = VT_EMPTY;
                IPropertyBag_Read(pPropBagCat, wszFriendlyName, &temp, NULL);
                TRACE("Considering category %s\n", debugstr_w(V_UNION(&temp, bstrVal)));
                VariantClear(&temp);
            }

            hrSub = IPropertyBag_Read(pPropBagCat, wszClsidName, &var, NULL);

            if (SUCCEEDED(hrSub))
                hrSub = CLSIDFromString(V_UNION(&var, bstrVal), &clsidCat);

            if (SUCCEEDED(hrSub))
                hrSub = ICreateDevEnum_CreateClassEnumerator(pCreateDevEnum, &clsidCat, &pEnum, 0);

            if (SUCCEEDED(hrSub))
            {
                while (IEnumMoniker_Next(pEnum, 1, &pMoniker, NULL) == S_OK)
                {
                    IPropertyBag * pPropBag = NULL;
                    REGFILTER2 rf2;
                    DWORD i;
                    BOOL bInputMatch = !bInputNeeded;
                    BOOL bOutputMatch = !bOutputNeeded;

                    ZeroMemory(&rf2, sizeof(rf2));

                    hrSub = IMoniker_BindToStorage(pMoniker, NULL, NULL, &IID_IPropertyBag, (LPVOID*)&pPropBag);

                    if (SUCCEEDED(hrSub))
                        hrSub = FM2_ReadFilterData(pPropBag, &rf2);

                    /* Logic used for bInputMatch expression:
                     * There exists some pin such that bInputNeeded implies (pin is an input and
                     * (bRender implies pin has render flag) and major/minor types members of
                     * pInputTypes )
                     * bOutputMatch is similar, but without the "bRender implies ..." part
                     * and substituting variables names containing input for output
                     */

                    /* determine whether filter meets requirements */
                    if (SUCCEEDED(hrSub) && (rf2.dwMerit >= dwMerit))
                    {
                        for (i = 0; (i < rf2.u.s1.cPins2) && (!bInputMatch || !bOutputMatch); i++)
                        {
                            const REGFILTERPINS2 * rfp2 = rf2.u.s1.rgPins2 + i;

                            bInputMatch = bInputMatch || (!(rfp2->dwFlags & REG_PINFLAG_B_OUTPUT) &&
                                (!bRender || (rfp2->dwFlags & REG_PINFLAG_B_RENDERER)) &&
                                MatchTypes(bExactMatch, rfp2->nMediaTypes, rfp2->lpMediaType, cInputTypes, pInputTypes));
                            bOutputMatch = bOutputMatch || ((rfp2->dwFlags & REG_PINFLAG_B_OUTPUT) &&
                                MatchTypes(bExactMatch, rfp2->nMediaTypes, rfp2->lpMediaType, cOutputTypes, pOutputTypes));
                        }

                        if (bInputMatch && bOutputMatch)
                        {
                            struct MONIKER_MERIT mm = {pMoniker, rf2.dwMerit};
                            IMoniker_AddRef(pMoniker);
                            add_data(&monikers, (LPBYTE)&mm, sizeof(mm));
                        }
                    }

                    FM2_DeleteRegFilter(&rf2);
                    if (pPropBag)
                        IPropertyBag_Release(pPropBag);
                    IMoniker_Release(pMoniker);
                }
                IEnumMoniker_Release(pEnum);
            }
        }

        VariantClear(&var);
        if (pPropBagCat)
            IPropertyBag_Release(pPropBagCat);
        IMoniker_Release(pMonikerCat);
    }


    if (SUCCEEDED(hr))
    {
        IMoniker ** ppMoniker;
        int i;
        ULONG nMonikerCount = monikers.current / sizeof(struct MONIKER_MERIT);

        /* sort the monikers in descending merit order */
        qsort(monikers.pData, nMonikerCount,
              sizeof(struct MONIKER_MERIT),
              mm_compare);

        /* construct an IEnumMoniker interface */
        ppMoniker = CoTaskMemAlloc(nMonikerCount * sizeof(IMoniker *));
        for (i = 0; i < nMonikerCount; i++)
        {
            /* no need to AddRef here as already AddRef'd above */
            ppMoniker[i] = ((struct MONIKER_MERIT *)monikers.pData)[i].pMoniker;
        }
        hr = EnumMonikerImpl_Create(ppMoniker, nMonikerCount, ppEnum);
        CoTaskMemFree(ppMoniker);
    }

    delete_vector(&monikers);
    IEnumMoniker_Release(pEnumCat);
    ICreateDevEnum_Release(pCreateDevEnum);

    return hr;
}

static ICOM_VTABLE(IFilterMapper2) fm2vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE

    FilterMapper2_QueryInterface,
    FilterMapper2_AddRef,
    FilterMapper2_Release,

    FilterMapper2_CreateCategory,
    FilterMapper2_UnregisterFilter,
    FilterMapper2_RegisterFilter,
    FilterMapper2_EnumMatchingFilters
};

/*** IUnknown methods ***/

static HRESULT WINAPI FilterMapper_QueryInterface(IFilterMapper * iface, REFIID riid, LPVOID *ppv)
{
    ICOM_THIS_From_IFilterMapper(FilterMapper2Impl, iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    return FilterMapper2_QueryInterface((IFilterMapper2*)&This->lpVtbl, riid, ppv);
}

static ULONG WINAPI FilterMapper_AddRef(IFilterMapper * iface)
{
    ICOM_THIS_From_IFilterMapper(FilterMapper2Impl, iface);

    return FilterMapper2_AddRef((IFilterMapper2*)This);
}

static ULONG WINAPI FilterMapper_Release(IFilterMapper * iface)
{
    ICOM_THIS_From_IFilterMapper(FilterMapper2Impl, iface);

    return FilterMapper2_Release((IFilterMapper2*)This);
}

/*** IFilterMapper methods ***/

static HRESULT WINAPI FilterMapper_EnumMatchingFilters(
    IFilterMapper * iface,
    IEnumRegFilters **ppEnum,
    DWORD dwMerit,
    BOOL bInputNeeded,
    CLSID clsInMaj,
    CLSID clsInSub,
    BOOL bRender,
    BOOL bOutputNeeded,
    CLSID clsOutMaj,
    CLSID clsOutSub)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI FilterMapper_RegisterFilter(IFilterMapper * iface, CLSID clsid, LPCWSTR szName, DWORD dwMerit)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI FilterMapper_RegisterFilterInstance(IFilterMapper * iface, CLSID clsid, LPCWSTR szName, CLSID *MRId)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI FilterMapper_RegisterPin(
    IFilterMapper * iface,
    CLSID Filter,
    LPCWSTR szName,
    BOOL bRendered,
    BOOL bOutput,
    BOOL bZero,
    BOOL bMany,
    CLSID ConnectsToFilter,
    LPCWSTR ConnectsToPin)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI FilterMapper_RegisterPinType(
    IFilterMapper * iface,
    CLSID clsFilter,
    LPCWSTR szName,
    CLSID clsMajorType,
    CLSID clsSubType)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI FilterMapper_UnregisterFilter(IFilterMapper * iface, CLSID Filter)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI FilterMapper_UnregisterFilterInstance(IFilterMapper * iface, CLSID MRId)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI FilterMapper_UnregisterPin(IFilterMapper * iface, CLSID Filter, LPCWSTR Name)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static ICOM_VTABLE(IFilterMapper) fmvtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE

    FilterMapper_QueryInterface,
    FilterMapper_AddRef,
    FilterMapper_Release,

    FilterMapper_RegisterFilter,
    FilterMapper_RegisterFilterInstance,
    FilterMapper_RegisterPin,
    FilterMapper_RegisterPinType,
    FilterMapper_UnregisterFilter,
    FilterMapper_UnregisterFilterInstance,
    FilterMapper_UnregisterPin,
    FilterMapper_EnumMatchingFilters
};
