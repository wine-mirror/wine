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

#include "comcat.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

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
    ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface);
    FIXME("(): stub\n");

    return E_NOTIMPL;
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
    ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface);
    FIXME("(): stub\n");

    return E_NOTIMPL;
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
    ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface);
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
    ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface);
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
    ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface);
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
    ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface);
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
