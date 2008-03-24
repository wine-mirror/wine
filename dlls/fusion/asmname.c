/*
 * IAssemblyName implementation
 *
 * Copyright 2008 James Hawkins
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
#define INITGUID

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "guiddef.h"
#include "fusion.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(fusion);

typedef struct {
    const IAssemblyNameVtbl *lpIAssemblyNameVtbl;

    LONG ref;
} IAssemblyNameImpl;

static HRESULT WINAPI IAssemblyNameImpl_QueryInterface(IAssemblyName *iface,
                                                       REFIID riid, LPVOID *ppobj)
{
    IAssemblyNameImpl *This = (IAssemblyNameImpl *)iface;

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAssemblyName))
    {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IAssemblyNameImpl_AddRef(IAssemblyName *iface)
{
    IAssemblyNameImpl *This = (IAssemblyNameImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IAssemblyNameImpl_Release(IAssemblyName *iface)
{
    IAssemblyNameImpl *This = (IAssemblyNameImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount + 1);

    if (!refCount)
        HeapFree(GetProcessHeap(), 0, This);

    return refCount;
}

static HRESULT WINAPI IAssemblyNameImpl_SetProperty(IAssemblyName *iface,
                                                    DWORD PropertyId,
                                                    LPVOID pvProperty,
                                                    DWORD cbProperty)
{
    FIXME("(%p, %d, %p, %d) stub!\n", iface, PropertyId, pvProperty, cbProperty);
    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyNameImpl_GetProperty(IAssemblyName *iface,
                                                    DWORD PropertyId,
                                                    LPVOID pvProperty,
                                                    LPDWORD pcbProperty)
{
    FIXME("(%p, %d, %p, %p) stub!\n", iface, PropertyId, pvProperty, pcbProperty);
    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyNameImpl_Finalize(IAssemblyName *iface)
{
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyNameImpl_GetDisplayName(IAssemblyName *iface,
                                                       LPOLESTR szDisplayName,
                                                       LPDWORD pccDisplayName,
                                                       DWORD dwDisplayFlags)
{
    FIXME("(%p, %s, %p, %d) stub!\n", iface, debugstr_w(szDisplayName),
          pccDisplayName, dwDisplayFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyNameImpl_Reserved(IAssemblyName *iface,
                                                 REFIID refIID,
                                                 IUnknown *pUnkReserved1,
                                                 IUnknown *pUnkReserved2,
                                                 LPCOLESTR szReserved,
                                                 LONGLONG llReserved,
                                                 LPVOID pvReserved,
                                                 DWORD cbReserved,
                                                 LPVOID *ppReserved)
{
    TRACE("(%p, %s, %p, %p, %s, %x%08x, %p, %d, %p)\n", iface,
          debugstr_guid(refIID), pUnkReserved1, pUnkReserved2,
          debugstr_w(szReserved), (DWORD)(llReserved >> 32), (DWORD)llReserved,
          pvReserved, cbReserved, ppReserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyNameImpl_GetName(IAssemblyName *iface,
                                                LPDWORD lpcwBuffer,
                                                WCHAR *pwzName)
{
    FIXME("(%p, %p, %p) stub!\n", iface, lpcwBuffer, pwzName);
    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyNameImpl_GetVersion(IAssemblyName *iface,
                                                   LPDWORD pdwVersionHi,
                                                   LPDWORD pdwVersionLow)
{
    FIXME("(%p, %p, %p) stub!\n", iface, pdwVersionHi, pdwVersionLow);
    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyNameImpl_IsEqual(IAssemblyName *iface,
                                                IAssemblyName *pName,
                                                DWORD dwCmpFlags)
{
    FIXME("(%p, %p, %d) stub!\n", iface, pName, dwCmpFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyNameImpl_Clone(IAssemblyName *iface,
                                              IAssemblyName **pName)
{
    FIXME("(%p, %p) stub!\n", iface, pName);
    return E_NOTIMPL;
}

static const IAssemblyNameVtbl AssemblyNameVtbl = {
    IAssemblyNameImpl_QueryInterface,
    IAssemblyNameImpl_AddRef,
    IAssemblyNameImpl_Release,
    IAssemblyNameImpl_SetProperty,
    IAssemblyNameImpl_GetProperty,
    IAssemblyNameImpl_Finalize,
    IAssemblyNameImpl_GetDisplayName,
    IAssemblyNameImpl_Reserved,
    IAssemblyNameImpl_GetName,
    IAssemblyNameImpl_GetVersion,
    IAssemblyNameImpl_IsEqual,
    IAssemblyNameImpl_Clone
};
