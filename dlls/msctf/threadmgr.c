/*
 *  ITfThreadMgr implementation
 *
 *  Copyright 2008 Aric Stewart, CodeWeavers
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

#include "wine/unicode.h"

#include "msctf.h"
#include "msctf_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctf);

typedef struct tagACLMulti {
    const ITfThreadMgrVtbl *ThreadMgrVtbl;
    LONG refCount;
} ThreadMgr;

static void ThreadMgr_Destructor(ThreadMgr *This)
{
    TRACE("destroying %p\n", This);
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI ThreadMgr_QueryInterface(ITfThreadMgr *iface, REFIID iid, LPVOID *ppvOut)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfThreadMgr))
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

static ULONG WINAPI ThreadMgr_AddRef(ITfThreadMgr *iface)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI ThreadMgr_Release(ITfThreadMgr *iface)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        ThreadMgr_Destructor(This);
    return ret;
}

/*****************************************************
 * ITfThreadMgr functions
 *****************************************************/

static HRESULT WINAPI ThreadMgr_fnActivate( ITfThreadMgr* iface, TfClientId *ptid)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_fnDeactivate( ITfThreadMgr* iface)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_CreateDocumentMgr( ITfThreadMgr* iface, ITfDocumentMgr
**ppdim)
{
    TRACE("(%p)\n",iface);
    return DocumentMgr_Constructor(ppdim);
}

static HRESULT WINAPI ThreadMgr_EnumDocumentMgrs( ITfThreadMgr* iface, IEnumTfDocumentMgrs
**ppEnum)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_GetFocus( ITfThreadMgr* iface, ITfDocumentMgr
**ppdimFocus)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_SetFocus( ITfThreadMgr* iface, ITfDocumentMgr *pdimFocus)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_AssociateFocus( ITfThreadMgr* iface, HWND hwnd,
ITfDocumentMgr *pdimNew, ITfDocumentMgr **ppdimPrev)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_IsThreadFocus( ITfThreadMgr* iface, BOOL *pfThreadFocus)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_GetFunctionProvider( ITfThreadMgr* iface, REFCLSID clsid,
ITfFunctionProvider **ppFuncProv)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_EnumFunctionProviders( ITfThreadMgr* iface,
IEnumTfFunctionProviders **ppEnum)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_GetGlobalCompartment( ITfThreadMgr* iface,
ITfCompartmentMgr **ppCompMgr)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static const ITfThreadMgrVtbl ThreadMgr_ThreadMgrVtbl =
{
    ThreadMgr_QueryInterface,
    ThreadMgr_AddRef,
    ThreadMgr_Release,

    ThreadMgr_fnActivate,
    ThreadMgr_fnDeactivate,
    ThreadMgr_CreateDocumentMgr,
    ThreadMgr_EnumDocumentMgrs,
    ThreadMgr_GetFocus,
    ThreadMgr_SetFocus,
    ThreadMgr_AssociateFocus,
    ThreadMgr_IsThreadFocus,
    ThreadMgr_GetFunctionProvider,
    ThreadMgr_EnumFunctionProviders,
    ThreadMgr_GetGlobalCompartment
};

HRESULT ThreadMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    ThreadMgr *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ThreadMgr));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->ThreadMgrVtbl= &ThreadMgr_ThreadMgrVtbl;
    This->refCount = 1;

    TRACE("returning %p\n", This);
    *ppOut = (IUnknown *)This;
    return S_OK;
}
