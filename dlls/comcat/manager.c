/*
 *	ComCatMgr IUnknown implementation for comcat.dll
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

#include "comcat_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static ULONG WINAPI COMCAT_IUnknown_AddRef(LPUNKNOWN iface);

/**********************************************************************
 * COMCAT_IUnknown_QueryInterface
 */
static HRESULT WINAPI COMCAT_IUnknown_QueryInterface(
    LPUNKNOWN iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    ICOM_THIS_MULTI(ComCatMgrImpl, unkVtbl, iface);
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown)) {
	*ppvObj = (LPVOID)&This->unkVtbl;
	COMCAT_IUnknown_AddRef(iface);
	return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ICatRegister)) {
	*ppvObj = (LPVOID)&This->regVtbl;
	COMCAT_IUnknown_AddRef(iface);
	return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ICatInformation)) {
	*ppvObj = (LPVOID)&This->infVtbl;
	COMCAT_IUnknown_AddRef(iface);
	return S_OK;
    }

    return E_NOINTERFACE;
}

/**********************************************************************
 * COMCAT_IUnknown_AddRef
 */
static ULONG WINAPI COMCAT_IUnknown_AddRef(LPUNKNOWN iface)
{
    ICOM_THIS_MULTI(ComCatMgrImpl, unkVtbl, iface);
    TRACE("\n");

    if (This == NULL) return E_POINTER;

    if (InterlockedIncrement(&This->ref) == 1) {
	InterlockedIncrement(&dll_ref);
    }
    return This->ref;
}

/**********************************************************************
 * COMCAT_IUnknown_Release
 */
static ULONG WINAPI COMCAT_IUnknown_Release(LPUNKNOWN iface)
{
    ICOM_THIS_MULTI(ComCatMgrImpl, unkVtbl, iface);
    TRACE("\n");

    if (This == NULL) return E_POINTER;

    if (InterlockedDecrement(&This->ref) == 0) {
	InterlockedDecrement(&dll_ref);
    }
    return This->ref;
}

/**********************************************************************
 * COMCAT_IUnknown_Vtbl
 */
static ICOM_VTABLE(IUnknown) COMCAT_IUnknown_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    COMCAT_IUnknown_QueryInterface,
    COMCAT_IUnknown_AddRef,
    COMCAT_IUnknown_Release
};

/**********************************************************************
 * static ComCatMgr instance
 */
ComCatMgrImpl COMCAT_ComCatMgr =
{
    &COMCAT_IUnknown_Vtbl,
    &COMCAT_ICatRegister_Vtbl,
    &COMCAT_ICatInformation_Vtbl,
    0
};
