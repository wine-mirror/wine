/*
 * Copyright 2025 Mike Kozelkov
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

#include "urlmon_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct {
    IUnknown        IUnknown_inner;
    IPersistFile    IPersistFile_iface;
    IZoneIdentifier IZoneIdentifier_iface;

    IUnknown        *outer;

    LONG ref;
} PersistentZoneIdentifier;

static inline PersistentZoneIdentifier *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, PersistentZoneIdentifier, IUnknown_inner);
}

static inline PersistentZoneIdentifier *impl_from_IPersistFile(IPersistFile *iface)
{
    return CONTAINING_RECORD(iface, PersistentZoneIdentifier, IPersistFile_iface);
}

static inline PersistentZoneIdentifier *impl_from_IZoneIdentifier(IZoneIdentifier *iface)
{
    return CONTAINING_RECORD(iface, PersistentZoneIdentifier, IZoneIdentifier_iface);
}

static HRESULT WINAPI PZIUnk_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    PersistentZoneIdentifier *This = impl_from_IUnknown(iface);

    *ppv = NULL;

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IUnknown_inner;
    }
    else if (IsEqualGUID(&IID_IPersist, riid))
    {
        TRACE("(%p)->(IID_IPersist %p)\n", This, ppv);
        *ppv = &This->IPersistFile_iface;
    }
    else if (IsEqualGUID(&IID_IPersistFile, riid))
    {
        TRACE("(%p)->(IID_IPersistFile %p)\n", This, ppv);
        *ppv = &This->IPersistFile_iface;
    }
    else if (IsEqualGUID(&IID_IZoneIdentifier, riid))
    {
        TRACE("(%p)->(IID_IZoneIdentifier %p)\n", This, ppv);
        *ppv = &This->IZoneIdentifier_iface;
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("not supported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI PZIUnk_AddRef(IUnknown *iface)
{
    PersistentZoneIdentifier *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI PZIUnk_Release(IUnknown *iface)
{
    PersistentZoneIdentifier *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        free(This);
        URLMON_UnlockModule();
    }

    return ref;
}

static const IUnknownVtbl PZIUnkVtbl = {
    PZIUnk_QueryInterface,
    PZIUnk_AddRef,
    PZIUnk_Release
};

static HRESULT WINAPI PZIPersistFile_QueryInterface(IPersistFile *iface, REFIID riid, void **ppv)
{
    PersistentZoneIdentifier *This = impl_from_IPersistFile(iface);

    TRACE("(%p, %s %p)\n", This, debugstr_guid(riid), ppv);

    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI PZIPersistFile_AddRef(IPersistFile *iface)
{
    PersistentZoneIdentifier *This = impl_from_IPersistFile(iface);

    TRACE("(%p)\n", This);

    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI PZIPersistFile_Release(IPersistFile *iface)
{
    PersistentZoneIdentifier *This = impl_from_IPersistFile(iface);

    TRACE("(%p)\n", This);

    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI PZIPersistFile_GetClassID(IPersistFile *iface, CLSID *clsid)
{
    PersistentZoneIdentifier *This = impl_from_IPersistFile(iface);

    FIXME("(%p, %p) not implemented\n", This, clsid);

    return E_NOTIMPL;
}

static HRESULT WINAPI PZIPersistFile_GetCurFile(IPersistFile *iface, LPOLESTR *file_name)
{
    PersistentZoneIdentifier *This = impl_from_IPersistFile(iface);

    FIXME("(%p, %p) not implemented\n", This, file_name);

    return E_NOTIMPL;
}

static HRESULT WINAPI PZIPersistFile_IsDirty(IPersistFile *iface)
{
    PersistentZoneIdentifier *This = impl_from_IPersistFile(iface);

    FIXME("(%p) not implemented\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI PZIPersistFile_Load(IPersistFile *iface, LPCOLESTR file_name, DWORD mode)
{
    PersistentZoneIdentifier *This = impl_from_IPersistFile(iface);

    FIXME("(%p, %s, 0x%08lx) not implemented\n", This, debugstr_w(file_name), mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI PZIPersistFile_Save(IPersistFile *iface, LPCOLESTR file_name, BOOL remember)
{
    PersistentZoneIdentifier *This = impl_from_IPersistFile(iface);

    FIXME("(%p, %s, %d) not implemented\n", This, debugstr_w(file_name), remember);

    return E_NOTIMPL;
}

static HRESULT WINAPI PZIPersistFile_SaveCompleted(IPersistFile* iface, LPCOLESTR pszFileName)
{
    PersistentZoneIdentifier *This = impl_from_IPersistFile(iface);

    FIXME("(%p, %p) not implemented\n", This, pszFileName);

    return E_NOTIMPL;
}

static const IPersistFileVtbl PZIPersistFileVtbl = {
    PZIPersistFile_QueryInterface,
    PZIPersistFile_AddRef,
    PZIPersistFile_Release,
    PZIPersistFile_GetClassID,
    PZIPersistFile_IsDirty,
    PZIPersistFile_Load,
    PZIPersistFile_Save,
    PZIPersistFile_SaveCompleted,
    PZIPersistFile_GetCurFile
};

static HRESULT WINAPI PZIZoneId_QueryInterface(IZoneIdentifier *iface, REFIID riid, void **ppv)
{
    PersistentZoneIdentifier *This = impl_from_IZoneIdentifier(iface);

    TRACE("(%p, %s %p)\n", This, debugstr_guid(riid), ppv);

    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI PZIZoneId_AddRef(IZoneIdentifier *iface)
{
    PersistentZoneIdentifier *This = impl_from_IZoneIdentifier(iface);

    TRACE("(%p)\n", This);

    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI PZIZoneId_Release(IZoneIdentifier *iface)
{
    PersistentZoneIdentifier *This = impl_from_IZoneIdentifier(iface);

    TRACE("(%p)\n", This);

    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI PZIZoneId_GetId(IZoneIdentifier* iface, DWORD* pdwZone)
{
    PersistentZoneIdentifier *This = impl_from_IZoneIdentifier(iface);

    FIXME("(%p, %p) not implemented\n", This, pdwZone);

    return E_NOTIMPL;
}

static HRESULT WINAPI PZIZoneId_Remove(IZoneIdentifier* iface)
{
    PersistentZoneIdentifier *This = impl_from_IZoneIdentifier(iface);

    FIXME("(%p) not implemented\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI PZIZoneId_SetId(IZoneIdentifier* iface, DWORD dwZone)
{
    PersistentZoneIdentifier *This = impl_from_IZoneIdentifier(iface);

    FIXME("(%p, 0x%08lx) not implemented\n", This, dwZone);

    return E_NOTIMPL;
}

static const IZoneIdentifierVtbl PZIZoneIdVtbl = {
    PZIZoneId_QueryInterface,
    PZIZoneId_AddRef,
    PZIZoneId_Release,
    PZIZoneId_GetId,
    PZIZoneId_SetId,
    PZIZoneId_Remove
};

HRESULT PersistentZoneIdentifier_Construct(IUnknown *outer, LPVOID *ppobj)
{

    PersistentZoneIdentifier *ret;

    TRACE("(%p %p)\n", outer, ppobj);


    ret = malloc(sizeof(PersistentZoneIdentifier));
    if (!ret)
        return E_OUTOFMEMORY;

    URLMON_LockModule();

    ret->IUnknown_inner.lpVtbl = &PZIUnkVtbl;
    ret->IPersistFile_iface.lpVtbl = &PZIPersistFileVtbl;
    ret->IZoneIdentifier_iface.lpVtbl = &PZIZoneIdVtbl;

    ret->ref = 1;
    ret->outer = outer ? outer : &ret->IUnknown_inner;

    *ppobj = &ret->IUnknown_inner;

    return S_OK;
}
