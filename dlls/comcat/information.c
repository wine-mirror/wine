/*
 *	ComCatMgr ICatInformation implementation for comcat.dll
 *
 * Copyright (C) 2002 John K. Hohm
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

#include <string.h>
#include "comcat.h"

#include "winuser.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static LPENUMCATEGORYINFO COMCAT_IEnumCATEGORYINFO_Construct(LCID lcid);
static HRESULT COMCAT_GetCategoryDesc(HKEY key, LCID lcid, PWCHAR pszDesc,
				      ULONG buf_wchars);

/**********************************************************************
 * COMCAT_ICatInformation_QueryInterface
 */
static HRESULT WINAPI COMCAT_ICatInformation_QueryInterface(
    LPCATINFORMATION iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface);
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    return IUnknown_QueryInterface((LPUNKNOWN)&This->unkVtbl, riid, ppvObj);
}

/**********************************************************************
 * COMCAT_ICatInformation_AddRef
 */
static ULONG WINAPI COMCAT_ICatInformation_AddRef(LPCATINFORMATION iface)
{
    ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface);
    TRACE("\n");

    if (This == NULL) return E_POINTER;

    return IUnknown_AddRef((LPUNKNOWN)&This->unkVtbl);
}

/**********************************************************************
 * COMCAT_ICatInformation_Release
 */
static ULONG WINAPI COMCAT_ICatInformation_Release(LPCATINFORMATION iface)
{
    ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface);
    TRACE("\n");

    if (This == NULL) return E_POINTER;

    return IUnknown_Release((LPUNKNOWN)&This->unkVtbl);
}

/**********************************************************************
 * COMCAT_ICatInformation_EnumCategories
 */
static HRESULT WINAPI COMCAT_ICatInformation_EnumCategories(
    LPCATINFORMATION iface,
    LCID lcid,
    LPENUMCATEGORYINFO *ppenumCatInfo)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface); */
    TRACE("\n");

    if (iface == NULL || ppenumCatInfo == NULL) return E_POINTER;

    *ppenumCatInfo = COMCAT_IEnumCATEGORYINFO_Construct(lcid);
    if (*ppenumCatInfo == NULL) return E_OUTOFMEMORY;
    IEnumCATEGORYINFO_AddRef(*ppenumCatInfo);
    return S_OK;
}

/**********************************************************************
 * COMCAT_ICatInformation_GetCategoryDesc
 */
static HRESULT WINAPI COMCAT_ICatInformation_GetCategoryDesc(
    LPCATINFORMATION iface,
    REFCATID rcatid,
    LCID lcid,
    PWCHAR *ppszDesc)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface); */
    WCHAR keyname[60] = { 'C', 'o', 'm', 'p', 'o', 'n', 'e', 'n',
			  't', ' ', 'C', 'a', 't', 'e', 'g', 'o',
			  'r', 'i', 'e', 's', '\\', 0 };
    HKEY key;
    HRESULT res;

    TRACE("\n\tCATID:\t%s\n\tLCID:\t%lX\n",debugstr_guid(rcatid), lcid);

    if (rcatid == NULL || ppszDesc == NULL) return E_INVALIDARG;

    /* Open the key for this category. */
    if (!StringFromGUID2(rcatid, keyname + 21, 39)) return E_FAIL;
    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, keyname, 0, KEY_READ, &key);
    if (res != ERROR_SUCCESS) return CAT_E_CATIDNOEXIST;

    /* Allocate a sensible amount of memory for the description. */
    *ppszDesc = (PWCHAR) CoTaskMemAlloc(128 * sizeof(WCHAR));
    if (*ppszDesc == NULL) {
	RegCloseKey(key);
	return E_OUTOFMEMORY;
    }

    /* Get the description, and make sure it's null terminated. */
    res = COMCAT_GetCategoryDesc(key, lcid, *ppszDesc, 128);
    RegCloseKey(key);
    if (FAILED(res)) {
	CoTaskMemFree(*ppszDesc);
	return res;
    }

    return S_OK;
}

/**********************************************************************
 * COMCAT_ICatInformation_EnumClassesOfCategories
 */
static HRESULT WINAPI COMCAT_ICatInformation_EnumClassesOfCategories(
    LPCATINFORMATION iface,
    ULONG cImplemented,
    CATID *rgcatidImpl,
    ULONG cRequired,
    CATID *rgcatidReq,
    LPENUMCLSID *ppenumCLSID)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface); */
    FIXME("(): stub\n");

    return E_NOTIMPL;
}

/**********************************************************************
 * COMCAT_ICatInformation_IsClassOfCategories
 */
static HRESULT WINAPI COMCAT_ICatInformation_IsClassOfCategories(
    LPCATINFORMATION iface,
    REFCLSID rclsid,
    ULONG cImplemented,
    CATID *rgcatidImpl,
    ULONG cRequired,
    CATID *rgcatidReq)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface); */
    FIXME("(): stub\n");

    return E_NOTIMPL;
}

/**********************************************************************
 * COMCAT_ICatInformation_EnumImplCategoriesOfClass
 */
static HRESULT WINAPI COMCAT_ICatInformation_EnumImplCategoriesOfClass(
    LPCATINFORMATION iface,
    REFCLSID rclsid,
    LPENUMCATID *ppenumCATID)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface); */
    FIXME("(): stub\n");

    return E_NOTIMPL;
}

/**********************************************************************
 * COMCAT_ICatInformation_EnumReqCategoriesOfClass
 */
static HRESULT WINAPI COMCAT_ICatInformation_EnumReqCategoriesOfClass(
    LPCATINFORMATION iface,
    REFCLSID rclsid,
    LPENUMCATID *ppenumCATID)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface); */
    FIXME("(): stub\n");

    return E_NOTIMPL;
}

/**********************************************************************
 * COMCAT_ICatInformation_Vtbl
 */
ICOM_VTABLE(ICatInformation) COMCAT_ICatInformation_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    COMCAT_ICatInformation_QueryInterface,
    COMCAT_ICatInformation_AddRef,
    COMCAT_ICatInformation_Release,
    COMCAT_ICatInformation_EnumCategories,
    COMCAT_ICatInformation_GetCategoryDesc,
    COMCAT_ICatInformation_EnumClassesOfCategories,
    COMCAT_ICatInformation_IsClassOfCategories,
    COMCAT_ICatInformation_EnumImplCategoriesOfClass,
    COMCAT_ICatInformation_EnumReqCategoriesOfClass
};

/**********************************************************************
 * IEnumCATEGORYINFO implementation
 *
 * This implementation is not thread-safe.  The manager itself is, but
 * I can't imagine a valid use of an enumerator in several threads.
 */
typedef struct
{
    ICOM_VFIELD(IEnumCATEGORYINFO);
    DWORD ref;
    LCID  lcid;
    HKEY  key;
    DWORD next_index;
} IEnumCATEGORYINFOImpl;

static ULONG WINAPI COMCAT_IEnumCATEGORYINFO_AddRef(LPENUMCATEGORYINFO iface)
{
    ICOM_THIS(IEnumCATEGORYINFOImpl, iface);
    TRACE("\n");

    if (This == NULL) return E_POINTER;

    return ++(This->ref);
}

static HRESULT WINAPI COMCAT_IEnumCATEGORYINFO_QueryInterface(
    LPENUMCATEGORYINFO iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    ICOM_THIS(IEnumCATEGORYINFOImpl, iface);
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
	IsEqualGUID(riid, &IID_IEnumCATEGORYINFO))
    {
	*ppvObj = (LPVOID)iface;
	COMCAT_IEnumCATEGORYINFO_AddRef(iface);
	return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI COMCAT_IEnumCATEGORYINFO_Release(LPENUMCATEGORYINFO iface)
{
    ICOM_THIS(IEnumCATEGORYINFOImpl, iface);
    TRACE("\n");

    if (This == NULL) return E_POINTER;

    if (--(This->ref) == 0) {
	if (This->key) RegCloseKey(This->key);
	HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return This->ref;
}

static HRESULT WINAPI COMCAT_IEnumCATEGORYINFO_Next(
    LPENUMCATEGORYINFO iface,
    ULONG celt,
    CATEGORYINFO *rgelt,
    ULONG *pceltFetched)
{
    ICOM_THIS(IEnumCATEGORYINFOImpl, iface);
    ULONG fetched = 0;

    TRACE("\n");

    if (This == NULL || rgelt == NULL) return E_POINTER;

    if (This->key) while (fetched < celt) {
	HRESULT res;
	WCHAR catid[39];
	DWORD cName = 39;
	HKEY subkey;

	res = RegEnumKeyExW(This->key, This->next_index, catid, &cName,
			    NULL, NULL, NULL, NULL);
	if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA) break;
	++(This->next_index);

	res = CLSIDFromString(catid, &rgelt->catid);
	if (FAILED(res)) continue;

	res = RegOpenKeyExW(This->key, catid, 0, KEY_READ, &subkey);
	if (res != ERROR_SUCCESS) continue;

	res = COMCAT_GetCategoryDesc(subkey, This->lcid,
				     rgelt->szDescription, 128);
	RegCloseKey(subkey);
	if (FAILED(res)) continue;

	rgelt->lcid = This->lcid;
	++fetched;
	++rgelt;
    }

    if (pceltFetched) *pceltFetched = fetched;
    return fetched == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI COMCAT_IEnumCATEGORYINFO_Skip(
    LPENUMCATEGORYINFO iface,
    ULONG celt)
{
/*     ICOM_THIS(IEnumCATEGORYINFOImpl, iface); */
    FIXME("(): stub\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI COMCAT_IEnumCATEGORYINFO_Reset(LPENUMCATEGORYINFO iface)
{
/*     ICOM_THIS(IEnumCATEGORYINFOImpl, iface); */
    FIXME("(): stub\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI COMCAT_IEnumCATEGORYINFO_Clone(
    LPENUMCATEGORYINFO iface,
    IEnumCATEGORYINFO **ppenum)
{
/*     ICOM_THIS(IEnumCATEGORYINFOImpl, iface); */
    FIXME("(): stub\n");

    return E_NOTIMPL;
}

ICOM_VTABLE(IEnumCATEGORYINFO) COMCAT_IEnumCATEGORYINFO_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    COMCAT_IEnumCATEGORYINFO_QueryInterface,
    COMCAT_IEnumCATEGORYINFO_AddRef,
    COMCAT_IEnumCATEGORYINFO_Release,
    COMCAT_IEnumCATEGORYINFO_Next,
    COMCAT_IEnumCATEGORYINFO_Skip,
    COMCAT_IEnumCATEGORYINFO_Reset,
    COMCAT_IEnumCATEGORYINFO_Clone
};

static LPENUMCATEGORYINFO COMCAT_IEnumCATEGORYINFO_Construct(LCID lcid)
{
    IEnumCATEGORYINFOImpl *This;

    This = (IEnumCATEGORYINFOImpl *) HeapAlloc(
	GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IEnumCATEGORYINFOImpl));
    if (This) {
	WCHAR keyname[21] = { 'C', 'o', 'm', 'p', 'o', 'n', 'e', 'n',
			      't', ' ', 'C', 'a', 't', 'e', 'g', 'o',
			      'r', 'i', 'e', 's', 0 };

	ICOM_VTBL(This) = &COMCAT_IEnumCATEGORYINFO_Vtbl;
	This->lcid = lcid;
	RegOpenKeyExW(HKEY_CLASSES_ROOT, keyname, 0, KEY_READ, &This->key);
    }
    return (LPENUMCATEGORYINFO)This;
}

/**********************************************************************
 * COMCAT_GetCategoryDesc
 */
static HRESULT COMCAT_GetCategoryDesc(HKEY key, LCID lcid, PWCHAR pszDesc,
				      ULONG buf_wchars)
{
    WCHAR fmt[4] = { '%', 'l', 'X', 0 };
    WCHAR valname[5];
    HRESULT res;
    DWORD type, size = (buf_wchars - 1) * sizeof(WCHAR);

    if (pszDesc == NULL) return E_INVALIDARG;

    /* FIXME: lcid comparisons are more complex than this! */
    wsprintfW(valname, fmt, lcid);
    res = RegQueryValueExW(key, valname, 0, &type, (LPBYTE)pszDesc, &size);
    if (res != ERROR_SUCCESS || type != REG_SZ) return CAT_E_NODESCRIPTION;
    pszDesc[size / sizeof(WCHAR)] = (WCHAR)0;

    return S_OK;
}
