/*
 *  ITfContext implementation
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

#include "wine/unicode.h"

#include "msctf.h"
#include "msctf_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctf);

typedef struct tagContext {
    const ITfContextVtbl *ContextVtbl;
    const ITfSourceVtbl *SourceVtbl;
    LONG refCount;

    TfClientId tidOwner;
    IUnknown *punk;  /* possible ITextStoreACP or ITfContextOwnerCompositionSink */
} Context;

static inline Context *impl_from_ITfSourceVtbl(ITfSource *iface)
{
    return (Context *)((char *)iface - FIELD_OFFSET(Context,SourceVtbl));
}

static void Context_Destructor(Context *This)
{
    TRACE("destroying %p\n", This);
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI Context_QueryInterface(ITfContext *iface, REFIID iid, LPVOID *ppvOut)
{
    Context *This = (Context *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfContext))
    {
        *ppvOut = This;
    }
    else if (IsEqualIID(iid, &IID_ITfSource))
    {
        *ppvOut = &This->SourceVtbl;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI Context_AddRef(ITfContext *iface)
{
    Context *This = (Context *)iface;
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI Context_Release(ITfContext *iface)
{
    Context *This = (Context *)iface;
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        Context_Destructor(This);
    return ret;
}

/*****************************************************
 * ITfContext functions
 *****************************************************/
static HRESULT WINAPI Context_RequestEditSession (ITfContext *iface,
        TfClientId tid, ITfEditSession *pes, DWORD dwFlags,
        HRESULT *phrSession)
{
    Context *This = (Context *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_InWriteSession (ITfContext *iface,
         TfClientId tid,
         BOOL *pfWriteSession)
{
    Context *This = (Context *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_GetSelection (ITfContext *iface,
        TfEditCookie ec, ULONG ulIndex, ULONG ulCount,
        TF_SELECTION *pSelection, ULONG *pcFetched)
{
    Context *This = (Context *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_SetSelection (ITfContext *iface,
        TfEditCookie ec, ULONG ulCount, const TF_SELECTION *pSelection)
{
    Context *This = (Context *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_GetStart (ITfContext *iface,
        TfEditCookie ec, ITfRange **ppStart)
{
    Context *This = (Context *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_GetEnd (ITfContext *iface,
        TfEditCookie ec, ITfRange **ppEnd)
{
    Context *This = (Context *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_GetActiveView (ITfContext *iface,
  ITfContextView **ppView)
{
    Context *This = (Context *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_EnumViews (ITfContext *iface,
        IEnumTfContextViews **ppEnum)
{
    Context *This = (Context *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_GetStatus (ITfContext *iface,
        TF_STATUS *pdcs)
{
    Context *This = (Context *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_GetProperty (ITfContext *iface,
        REFGUID guidProp, ITfProperty **ppProp)
{
    Context *This = (Context *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_GetAppProperty (ITfContext *iface,
        REFGUID guidProp, ITfReadOnlyProperty **ppProp)
{
    Context *This = (Context *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_TrackProperties (ITfContext *iface,
        const GUID **prgProp, ULONG cProp, const GUID **prgAppProp,
        ULONG cAppProp, ITfReadOnlyProperty **ppProperty)
{
    Context *This = (Context *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_EnumProperties (ITfContext *iface,
        IEnumTfProperties **ppEnum)
{
    Context *This = (Context *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_GetDocumentMgr (ITfContext *iface,
        ITfDocumentMgr **ppDm)
{
    Context *This = (Context *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_CreateRangeBackup (ITfContext *iface,
        TfEditCookie ec, ITfRange *pRange, ITfRangeBackup **ppBackup)
{
    Context *This = (Context *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static const ITfContextVtbl Context_ContextVtbl =
{
    Context_QueryInterface,
    Context_AddRef,
    Context_Release,

    Context_RequestEditSession,
    Context_InWriteSession,
    Context_GetSelection,
    Context_SetSelection,
    Context_GetStart,
    Context_GetEnd,
    Context_GetActiveView,
    Context_EnumViews,
    Context_GetStatus,
    Context_GetProperty,
    Context_GetAppProperty,
    Context_TrackProperties,
    Context_EnumProperties,
    Context_GetDocumentMgr,
    Context_CreateRangeBackup
};

static HRESULT WINAPI Source_QueryInterface(ITfSource *iface, REFIID iid, LPVOID *ppvOut)
{
    Context *This = impl_from_ITfSourceVtbl(iface);
    return Context_QueryInterface((ITfContext *)This, iid, *ppvOut);
}

static ULONG WINAPI Source_AddRef(ITfSource *iface)
{
    Context *This = impl_from_ITfSourceVtbl(iface);
    return Context_AddRef((ITfContext *)This);
}

static ULONG WINAPI Source_Release(ITfSource *iface)
{
    Context *This = impl_from_ITfSourceVtbl(iface);
    return Context_Release((ITfContext *)This);
}

/*****************************************************
 * ITfSource functions
 *****************************************************/
static WINAPI HRESULT ContextSource_AdviseSink(ITfSource *iface,
        REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
    Context *This = impl_from_ITfSourceVtbl(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static WINAPI HRESULT ContextSource_UnadviseSink(ITfSource *iface, DWORD pdwCookie)
{
    Context *This = impl_from_ITfSourceVtbl(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static const ITfSourceVtbl Context_SourceVtbl =
{
    Source_QueryInterface,
    Source_AddRef,
    Source_Release,

    ContextSource_AdviseSink,
    ContextSource_UnadviseSink,
};

HRESULT Context_Constructor(TfClientId tidOwner, IUnknown *punk, ITfContext **ppOut, TfEditCookie *pecTextStore)
{
    Context *This;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(Context));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->ContextVtbl= &Context_ContextVtbl;
    This->SourceVtbl = &Context_SourceVtbl;
    This->refCount = 1;
    This->tidOwner = tidOwner;
    This->punk = punk;

    TRACE("returning %p\n", This);
    *ppOut = (ITfContext*)This;
    /* FIXME */
    *pecTextStore = 0xdeaddead;
    return S_OK;
}
