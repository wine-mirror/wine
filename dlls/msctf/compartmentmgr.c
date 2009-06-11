/*
 *  ITfCompartmentMgr implementation
 *
 *  Copyright 2009 Aric Stewart, CodeWeavers
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "shlwapi.h"
#include "winerror.h"
#include "objbase.h"
#include "oleauto.h"

#include "wine/unicode.h"
#include "wine/list.h"

#include "msctf.h"
#include "msctf_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctf);

typedef struct tagCompartmentValue {
    struct list entry;
    GUID guid;
    TfClientId owner;
    ITfCompartment *compartment;
} CompartmentValue;

typedef struct tagCompartmentMgr {
    const ITfCompartmentMgrVtbl *CompartmentMgrVtbl;
    LONG refCount;

    IUnknown *pUnkOuter;

    struct list values;
} CompartmentMgr;


HRESULT CompartmentMgr_Destructor(ITfCompartmentMgr *iface)
{
    CompartmentMgr *This = (CompartmentMgr *)iface;
    struct list *cursor, *cursor2;

    LIST_FOR_EACH_SAFE(cursor, cursor2, &This->values)
    {
        CompartmentValue* value = LIST_ENTRY(cursor,CompartmentValue,entry);
        list_remove(cursor);
        ITfCompartment_Release(value->compartment);
        HeapFree(GetProcessHeap(),0,value);
    }

    HeapFree(GetProcessHeap(),0,This);
    return S_OK;
}

/*****************************************************
 * ITfCompartmentMgr functions
 *****************************************************/
static HRESULT WINAPI CompartmentMgr_QueryInterface(ITfCompartmentMgr *iface, REFIID iid, LPVOID *ppvOut)
{
    CompartmentMgr *This = (CompartmentMgr *)iface;
    if (This->pUnkOuter)
        return IUnknown_QueryInterface(This->pUnkOuter, iid, *ppvOut);
    else
    {
        *ppvOut = NULL;

        if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfCompartmentMgr))
        {
            *ppvOut = This;
        }

        if (*ppvOut)
        {
            IUnknown_AddRef(iface);
            return S_OK;
        }

        WARN("unsupported interface: %s\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI CompartmentMgr_AddRef(ITfCompartmentMgr *iface)
{
    CompartmentMgr *This = (CompartmentMgr *)iface;
    if (This->pUnkOuter)
        return IUnknown_AddRef(This->pUnkOuter);
    else
        return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI CompartmentMgr_Release(ITfCompartmentMgr *iface)
{
    CompartmentMgr *This = (CompartmentMgr *)iface;
    if (This->pUnkOuter)
        return IUnknown_Release(This->pUnkOuter);
    else
    {
        ULONG ret;

        ret = InterlockedDecrement(&This->refCount);
        if (ret == 0)
            CompartmentMgr_Destructor(iface);
        return ret;
    }
}

HRESULT WINAPI CompartmentMgr_GetCompartment(ITfCompartmentMgr *iface,
        REFGUID rguid, ITfCompartment **ppcomp)
{
    CompartmentMgr *This = (CompartmentMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

HRESULT WINAPI CompartmentMgr_ClearCompartment(ITfCompartmentMgr *iface,
    TfClientId tid, REFGUID rguid)
{
    CompartmentMgr *This = (CompartmentMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

HRESULT WINAPI CompartmentMgr_EnumCompartments(ITfCompartmentMgr *iface,
 IEnumGUID **ppEnum)
{
    CompartmentMgr *This = (CompartmentMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static const ITfCompartmentMgrVtbl CompartmentMgr_CompartmentMgrVtbl =
{
    CompartmentMgr_QueryInterface,
    CompartmentMgr_AddRef,
    CompartmentMgr_Release,

    CompartmentMgr_GetCompartment,
    CompartmentMgr_ClearCompartment,
    CompartmentMgr_EnumCompartments
};

HRESULT CompartmentMgr_Constructor(IUnknown *pUnkOuter, REFIID riid, IUnknown **ppOut)
{
    CompartmentMgr *This;

    if (!ppOut)
        return E_POINTER;

    if (pUnkOuter && !IsEqualIID (riid, &IID_IUnknown))
        return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(CompartmentMgr));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->CompartmentMgrVtbl = &CompartmentMgr_CompartmentMgrVtbl;
    This->pUnkOuter = pUnkOuter;
    list_init(&This->values);

    if (pUnkOuter)
    {
        TRACE("returning %p\n", This);
        *ppOut = (IUnknown*)This;
        return S_OK;
    }
    else
    {
        HRESULT hr;
        hr = IUnknown_QueryInterface((IUnknown*)This, riid, (LPVOID*)ppOut);
        if (FAILED(hr))
            HeapFree(GetProcessHeap(),0,This);
        return hr;
    }
}
