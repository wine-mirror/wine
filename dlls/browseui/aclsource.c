/*
 *	Shell AutoComplete list
 *
 *	Copyright 2008	CodeWeavers, Aric Stewart
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

#include "shlguid.h"
#include "shlobj.h"

#include "browseui.h"

WINE_DEFAULT_DEBUG_CHANNEL(browseui);

typedef struct tagACLShellSource {
    IEnumString IEnumString_iface;
    IACList2 IACList2_iface;
    LONG refCount;
    DWORD dwOptions;
} ACLShellSource;

static inline ACLShellSource *impl_from_IACList2(IACList2 *iface)
{
    return CONTAINING_RECORD(iface, ACLShellSource, IACList2_iface);
}

static inline ACLShellSource *impl_from_IEnumString(IEnumString *iface)
{
    return CONTAINING_RECORD(iface, ACLShellSource, IEnumString_iface);
}

static HRESULT WINAPI ACLShellSource_QueryInterface(IEnumString *iface, REFIID iid, LPVOID *ppvOut)
{
    ACLShellSource *This = impl_from_IEnumString(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(iid), ppvOut);

    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IEnumString))
    {
        *ppvOut = &This->IEnumString_iface;
    }
    else if (IsEqualIID(iid, &IID_IACList2) || IsEqualIID(iid, &IID_IACList))
    {
        *ppvOut = &This->IACList2_iface;
    }

    if (*ppvOut)
    {
        IEnumString_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ACLShellSource_AddRef(IEnumString *iface)
{
    ACLShellSource *This = impl_from_IEnumString(iface);
    ULONG ref = InterlockedIncrement(&This->refCount);
    TRACE("(%p)->(%lu)\n", This, ref);
    return ref;
}

static ULONG WINAPI ACLShellSource_Release(IEnumString *iface)
{
    ACLShellSource *This = impl_from_IEnumString(iface);
    ULONG ref = InterlockedDecrement(&This->refCount);

    TRACE("(%p)->(%lu)\n", This, ref);

    if (ref == 0)
        free(This);
    return ref;
}

static HRESULT WINAPI ACLShellSource_Next(IEnumString *iface, ULONG celt, LPOLESTR *rgelt,
    ULONG *fetched)
{
    ACLShellSource *This = impl_from_IEnumString(iface);
    FIXME("(%p)->(%lu %p %p): stub\n", This, celt, rgelt, fetched);
    return E_NOTIMPL;
}

static HRESULT WINAPI ACLShellSource_Skip(IEnumString *iface, ULONG celt)
{
    ACLShellSource *This = impl_from_IEnumString(iface);
    FIXME("(%p)->(%lu): stub\n", This, celt);
    return E_NOTIMPL;
}

static HRESULT WINAPI ACLShellSource_Reset(IEnumString *iface)
{
    ACLShellSource *This = impl_from_IEnumString(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ACLShellSource_Clone(IEnumString *iface, IEnumString **ppenum)
{
    ACLShellSource *This = impl_from_IEnumString(iface);
    FIXME("(%p)->(%p): stub\n", This, ppenum);
    return E_NOTIMPL;
}

static const IEnumStringVtbl ACLShellSourceVtbl = {
    ACLShellSource_QueryInterface,
    ACLShellSource_AddRef,
    ACLShellSource_Release,
    ACLShellSource_Next,
    ACLShellSource_Skip,
    ACLShellSource_Reset,
    ACLShellSource_Clone
};

static HRESULT WINAPI ACList_QueryInterface(IACList2 *iface, REFIID iid, void **ppvOut)
{
    ACLShellSource *This = impl_from_IACList2(iface);
    return IEnumString_QueryInterface(&This->IEnumString_iface, iid, ppvOut);
}

static ULONG WINAPI ACList_AddRef(IACList2 *iface)
{
    ACLShellSource *This = impl_from_IACList2(iface);
    return IEnumString_AddRef(&This->IEnumString_iface);
}

static ULONG WINAPI ACList_Release(IACList2 *iface)
{
    ACLShellSource *This = impl_from_IACList2(iface);
    return IEnumString_Release(&This->IEnumString_iface);
}

static HRESULT WINAPI ACList_Expand(IACList2 *iface, LPCWSTR wstr)
{
    ACLShellSource *This = impl_from_IACList2(iface);
    FIXME("STUB:(%p) %s\n",This,debugstr_w(wstr));
    return E_NOTIMPL;
}

static HRESULT WINAPI ACList_GetOptions(IACList2 *iface, DWORD *pdwFlag)
{
    ACLShellSource *This = impl_from_IACList2(iface);
    *pdwFlag = This->dwOptions;
    return S_OK;
}

static HRESULT WINAPI ACList_SetOptions(IACList2 *iface,
    DWORD dwFlag)
{
    ACLShellSource *This = impl_from_IACList2(iface);
    This->dwOptions = dwFlag;
    return S_OK;
}

static const IACList2Vtbl ACListVtbl =
{
    ACList_QueryInterface,
    ACList_AddRef,
    ACList_Release,
    ACList_Expand,
    ACList_SetOptions,
    ACList_GetOptions
};

HRESULT ACLShellSource_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    ACLShellSource *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    if (!(This = calloc(1, sizeof(*This))))
        return E_OUTOFMEMORY;

    This->IEnumString_iface.lpVtbl = &ACLShellSourceVtbl;
    This->IACList2_iface.lpVtbl = &ACListVtbl;
    This->refCount = 1;

    TRACE("returning %p\n", This);
    *ppOut = (IUnknown *)&This->IEnumString_iface;
    return S_OK;
}
