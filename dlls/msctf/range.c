/*
 *  ITfRange implementation
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

#include "msctf.h"
#include "msctf_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctf);

typedef struct tagRange {
    ITfRangeACP ITfRangeACP_iface;
    LONG refCount;

    ITfContext *context;

    TfGravity gravityStart, gravityEnd;
    DWORD anchorStart, anchorEnd;

} Range;

static inline Range *impl_from_ITfRangeACP(ITfRangeACP *iface)
{
    return CONTAINING_RECORD(iface, Range, ITfRangeACP_iface);
}

static Range *unsafe_impl_from_ITfRange(ITfRange *iface)
{
    return CONTAINING_RECORD(iface, Range, ITfRangeACP_iface);
}

static void Range_Destructor(Range *This)
{
    TRACE("destroying %p\n", This);
    ITfContext_Release(This->context);
    free(This);
}

static HRESULT WINAPI Range_QueryInterface(ITfRangeACP *iface, REFIID iid, LPVOID *ppvOut)
{
    Range *range = impl_from_ITfRangeACP(iface);

    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) ||
            IsEqualIID(iid, &IID_ITfRange) ||
            IsEqualIID(iid, &IID_ITfRangeACP))
    {
        *ppvOut = &range->ITfRangeACP_iface;
    }

    if (*ppvOut)
    {
        ITfRangeACP_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI Range_AddRef(ITfRangeACP *iface)
{
    Range *range = impl_from_ITfRangeACP(iface);
    return InterlockedIncrement(&range->refCount);
}

static ULONG WINAPI Range_Release(ITfRangeACP *iface)
{
    Range *range = impl_from_ITfRangeACP(iface);
    ULONG ret;

    ret = InterlockedDecrement(&range->refCount);
    if (ret == 0)
        Range_Destructor(range);
    return ret;
}

static HRESULT WINAPI Range_GetText(ITfRangeACP *iface, TfEditCookie ec,
        DWORD dwFlags, WCHAR *pchText, ULONG cchMax, ULONG *pcch)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_SetText(ITfRangeACP *iface, TfEditCookie ec,
         DWORD dwFlags, const WCHAR *pchText, LONG cch)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_GetFormattedText(ITfRangeACP *iface, TfEditCookie ec,
        IDataObject **ppDataObject)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_GetEmbedded(ITfRangeACP *iface, TfEditCookie ec,
        REFGUID rguidService, REFIID riid, IUnknown **ppunk)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_InsertEmbedded(ITfRangeACP *iface, TfEditCookie ec,
        DWORD dwFlags, IDataObject *pDataObject)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_ShiftStart(ITfRangeACP *iface, TfEditCookie ec,
        LONG cchReq, LONG *pcch, const TF_HALTCOND *pHalt)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_ShiftEnd(ITfRangeACP *iface, TfEditCookie ec,
        LONG cchReq, LONG *pcch, const TF_HALTCOND *pHalt)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_ShiftStartToRange(ITfRangeACP *iface, TfEditCookie ec,
        ITfRange *pRange, TfAnchor aPos)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_ShiftEndToRange(ITfRangeACP *iface, TfEditCookie ec,
        ITfRange *pRange, TfAnchor aPos)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_ShiftStartRegion(ITfRangeACP *iface, TfEditCookie ec,
        TfShiftDir dir, BOOL *pfNoRegion)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_ShiftEndRegion(ITfRangeACP *iface, TfEditCookie ec,
        TfShiftDir dir, BOOL *pfNoRegion)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_IsEmpty(ITfRangeACP *iface, TfEditCookie ec,
        BOOL *pfEmpty)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_Collapse(ITfRangeACP *iface, TfEditCookie ec,
        TfAnchor aPos)
{
    Range *range = impl_from_ITfRangeACP(iface);

    TRACE("%p, %li, %i.\n", iface, ec, aPos);

    switch (aPos)
    {
        case TF_ANCHOR_START:
            range->anchorEnd = range->anchorStart;
            break;
        case TF_ANCHOR_END:
            range->anchorStart = range->anchorEnd;
            break;
        default:
            return E_INVALIDARG;
    }

    return S_OK;
}

static HRESULT WINAPI Range_IsEqualStart(ITfRangeACP *iface, TfEditCookie ec,
        ITfRange *pWith, TfAnchor aPos, BOOL *pfEqual)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_IsEqualEnd(ITfRangeACP *iface, TfEditCookie ec,
        ITfRange *pWith, TfAnchor aPos, BOOL *pfEqual)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_CompareStart(ITfRangeACP *iface, TfEditCookie ec,
        ITfRange *pWith, TfAnchor aPos, LONG *plResult)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_CompareEnd(ITfRangeACP *iface, TfEditCookie ec,
        ITfRange *pWith, TfAnchor aPos, LONG *plResult)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_AdjustForInsert(ITfRangeACP *iface, TfEditCookie ec,
        ULONG cchInsert, BOOL *pfInsertOk)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_GetGravity(ITfRangeACP *iface,
        TfGravity *pgStart, TfGravity *pgEnd)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_SetGravity(ITfRangeACP *iface, TfEditCookie ec,
         TfGravity gStart, TfGravity gEnd)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_Clone(ITfRangeACP *iface, ITfRange **ppClone)
{
    FIXME("STUB:(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_GetContext(ITfRangeACP *iface, ITfContext **context)
{
    Range *range = impl_from_ITfRangeACP(iface);

    TRACE("%p, %p.\n", iface, context);

    if (!context)
        return E_INVALIDARG;

    *context = range->context;
    ITfContext_AddRef(*context);

    return S_OK;
}

static HRESULT WINAPI Range_GetExtent(ITfRangeACP *iface, LONG *anchor, LONG *count)
{
    FIXME("%p, %p, %p.\n", iface, anchor, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI Range_SetExtent(ITfRangeACP *iface, LONG anchor, LONG count)
{
    FIXME("%p, %ld, %ld.\n", iface, anchor, count);

    return E_NOTIMPL;
}

static const ITfRangeACPVtbl rangevtbl =
{
    Range_QueryInterface,
    Range_AddRef,
    Range_Release,
    Range_GetText,
    Range_SetText,
    Range_GetFormattedText,
    Range_GetEmbedded,
    Range_InsertEmbedded,
    Range_ShiftStart,
    Range_ShiftEnd,
    Range_ShiftStartToRange,
    Range_ShiftEndToRange,
    Range_ShiftStartRegion,
    Range_ShiftEndRegion,
    Range_IsEmpty,
    Range_Collapse,
    Range_IsEqualStart,
    Range_IsEqualEnd,
    Range_CompareStart,
    Range_CompareEnd,
    Range_AdjustForInsert,
    Range_GetGravity,
    Range_SetGravity,
    Range_Clone,
    Range_GetContext,
    Range_GetExtent,
    Range_SetExtent,
};

HRESULT Range_Constructor(ITfContext *context, DWORD anchorStart, DWORD anchorEnd, ITfRange **ppOut)
{
    Range *This;

    This = calloc(1, sizeof(Range));
    if (This == NULL)
        return E_OUTOFMEMORY;

    TRACE("(%p) %p\n", This, context);

    This->ITfRangeACP_iface.lpVtbl = &rangevtbl;
    This->refCount = 1;
    This->context = context;
    ITfContext_AddRef(This->context);
    This->anchorStart = anchorStart;
    This->anchorEnd = anchorEnd;

    *ppOut = (ITfRange *)&This->ITfRangeACP_iface;

    TRACE("returning %p\n", *ppOut);

    return S_OK;
}

/* Internal conversion functions */

HRESULT TF_SELECTION_to_TS_SELECTION_ACP(const TF_SELECTION *tf, TS_SELECTION_ACP *tsAcp)
{
    Range *This;

    if (!tf || !tsAcp || !tf->range)
        return E_INVALIDARG;

    This = unsafe_impl_from_ITfRange(tf->range);

    tsAcp->acpStart = This->anchorStart;
    tsAcp->acpEnd = This->anchorEnd;
    tsAcp->style.ase = (TsActiveSelEnd)tf->style.ase;
    tsAcp->style.fInterimChar = tf->style.fInterimChar;
    return S_OK;
}
