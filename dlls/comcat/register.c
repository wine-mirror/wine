/*
 *	ComCatMgr ICatRegister implementation for comcat.dll
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
 * COMCAT_ICatRegister_QueryInterface
 */
static HRESULT WINAPI COMCAT_ICatRegister_QueryInterface(
    LPCATREGISTER iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface);
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    return IUnknown_QueryInterface((LPUNKNOWN)&This->unkVtbl, riid, ppvObj);
}

/**********************************************************************
 * COMCAT_ICatRegister_AddRef
 */
static ULONG WINAPI COMCAT_ICatRegister_AddRef(LPCATREGISTER iface)
{
    ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface);
    TRACE("\n");

    if (This == NULL) return E_POINTER;

    return IUnknown_AddRef((LPUNKNOWN)&This->unkVtbl);
}

/**********************************************************************
 * COMCAT_ICatRegister_Release
 */
static ULONG WINAPI COMCAT_ICatRegister_Release(LPCATREGISTER iface)
{
    ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface);
    TRACE("\n");

    if (This == NULL) return E_POINTER;

    return IUnknown_Release((LPUNKNOWN)&This->unkVtbl);
}

/**********************************************************************
 * COMCAT_ICatRegister_RegisterCategories
 */
static HRESULT WINAPI COMCAT_ICatRegister_RegisterCategories(
    LPCATREGISTER iface,
    ULONG cCategories,
    CATEGORYINFO *rgCategoryInfo)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface); */
    FIXME("(): stub\n");

    return E_NOTIMPL;
}

/**********************************************************************
 * COMCAT_ICatRegister_UnRegisterCategories
 */
static HRESULT WINAPI COMCAT_ICatRegister_UnRegisterCategories(
    LPCATREGISTER iface,
    ULONG cCategories,
    CATID *rgcatid)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface); */
    FIXME("(): stub\n");

    return E_NOTIMPL;
}

/**********************************************************************
 * COMCAT_ICatRegister_RegisterClassImplCategories
 */
static HRESULT WINAPI COMCAT_ICatRegister_RegisterClassImplCategories(
    LPCATREGISTER iface,
    REFCLSID rclsid,
    ULONG cCategories,
    CATID *rgcatid)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface); */
    FIXME("(): stub\n");

    return E_NOTIMPL;
}

/**********************************************************************
 * COMCAT_ICatRegister_UnRegisterClassImplCategories
 */
static HRESULT WINAPI COMCAT_ICatRegister_UnRegisterClassImplCategories(
    LPCATREGISTER iface,
    REFCLSID rclsid,
    ULONG cCategories,
    CATID *rgcatid)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface); */
    FIXME("(): stub\n");

    return E_NOTIMPL;
}

/**********************************************************************
 * COMCAT_ICatRegister_RegisterClassReqCategories
 */
static HRESULT WINAPI COMCAT_ICatRegister_RegisterClassReqCategories(
    LPCATREGISTER iface,
    REFCLSID rclsid,
    ULONG cCategories,
    CATID *rgcatid)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface); */
    FIXME("(): stub\n");

    return E_NOTIMPL;
}

/**********************************************************************
 * COMCAT_ICatRegister_UnRegisterClassReqCategories
 */
static HRESULT WINAPI COMCAT_ICatRegister_UnRegisterClassReqCategories(
    LPCATREGISTER iface,
    REFCLSID rclsid,
    ULONG cCategories,
    CATID *rgcatid)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface); */
    FIXME("(): stub\n");

    return E_NOTIMPL;
}

/**********************************************************************
 * COMCAT_ICatRegister_Vtbl
 */
ICOM_VTABLE(ICatRegister) COMCAT_ICatRegister_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    COMCAT_ICatRegister_QueryInterface,
    COMCAT_ICatRegister_AddRef,
    COMCAT_ICatRegister_Release,
    COMCAT_ICatRegister_RegisterCategories,
    COMCAT_ICatRegister_UnRegisterCategories,
    COMCAT_ICatRegister_RegisterClassImplCategories,
    COMCAT_ICatRegister_UnRegisterClassImplCategories,
    COMCAT_ICatRegister_RegisterClassReqCategories,
    COMCAT_ICatRegister_UnRegisterClassReqCategories
};
