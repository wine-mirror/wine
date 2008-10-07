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

#include "shlguid.h"
#include "shlobj.h"

#include "wine/unicode.h"

#include "browseui.h"

WINE_DEFAULT_DEBUG_CHANNEL(browseui);

typedef struct tagACLMulti {
    const IACList2Vtbl *acl2Vtbl;
    LONG refCount;
    DWORD dwOptions;
} ACLShellSource;

static void ACLShellSource_Destructor(ACLShellSource *This)
{
    TRACE("destroying %p\n", This);
    heap_free(This);
}

static HRESULT WINAPI ACLShellSource_QueryInterface(IACList2 *iface, REFIID iid, LPVOID *ppvOut)
{
    ACLShellSource *This = (ACLShellSource *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IACList2) ||
        IsEqualIID(iid, &IID_IACList))
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

static ULONG WINAPI ACLShellSource_AddRef(IACList2 *iface)
{
    ACLShellSource *This = (ACLShellSource *)iface;
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI ACLShellSource_Release(IACList2 *iface)
{
    ACLShellSource *This = (ACLShellSource *)iface;
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        ACLShellSource_Destructor(This);
    return ret;
}

static HRESULT WINAPI ACLShellSource_Expand(IACList2 *iface, LPCWSTR wstr)
{
    ACLShellSource *This = (ACLShellSource *)iface;
    FIXME("STUB:(%p) %s\n",This,debugstr_w(wstr));
    return E_NOTIMPL;
}


static HRESULT WINAPI ACLShellSource_GetOptions(IACList2 *iface,
    DWORD *pdwFlag)
{
    ACLShellSource *This = (ACLShellSource *)iface;
    *pdwFlag = This->dwOptions;
    return S_OK;
}

static HRESULT WINAPI ACLShellSource_SetOptions(IACList2 *iface,
    DWORD dwFlag)
{
    ACLShellSource *This = (ACLShellSource *)iface;
    This->dwOptions = dwFlag;
    return S_OK;
}

static const IACList2Vtbl ACLMulti_ACList2Vtbl =
{
    ACLShellSource_QueryInterface,
    ACLShellSource_AddRef,
    ACLShellSource_Release,

    ACLShellSource_Expand,

    ACLShellSource_SetOptions,
    ACLShellSource_GetOptions
};

HRESULT ACLShellSource_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    ACLShellSource *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = heap_alloc_zero(sizeof(ACLShellSource));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->acl2Vtbl = &ACLMulti_ACList2Vtbl;
    This->refCount = 1;

    TRACE("returning %p\n", This);
    *ppOut = (IUnknown *)This;
    return S_OK;
}
