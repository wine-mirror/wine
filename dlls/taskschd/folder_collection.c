/*
 * Copyright 2014 Dmitry Timoshkov
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

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "taskschd.h"
#include "taskschd_private.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(taskschd);

typedef struct
{
    ITaskFolderCollection ITaskFolderCollection_iface;
    LONG ref;
} TaskFolderCollection;

static inline TaskFolderCollection *impl_from_ITaskFolderCollection(ITaskFolderCollection *iface)
{
    return CONTAINING_RECORD(iface, TaskFolderCollection, ITaskFolderCollection_iface);
}

static ULONG WINAPI folders_AddRef(ITaskFolderCollection *iface)
{
    TaskFolderCollection *folders = impl_from_ITaskFolderCollection(iface);
    return InterlockedIncrement(&folders->ref);
}

static ULONG WINAPI folders_Release(ITaskFolderCollection *iface)
{
    TaskFolderCollection *folders = impl_from_ITaskFolderCollection(iface);
    LONG ref = InterlockedDecrement(&folders->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        heap_free(folders);
    }

    return ref;
}

static HRESULT WINAPI folders_QueryInterface(ITaskFolderCollection *iface, REFIID riid, void **obj)
{
    if (!riid || !obj) return E_INVALIDARG;

    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_ITaskFolderCollection) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        ITaskFolderCollection_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static HRESULT WINAPI folders_GetTypeInfoCount(ITaskFolderCollection *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI folders_GetTypeInfo(ITaskFolderCollection *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%u,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI folders_GetIDsOfNames(ITaskFolderCollection *iface, REFIID riid, LPOLESTR *names,
                                                UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%u,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI folders_Invoke(ITaskFolderCollection *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                         DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%d,%s,%04x,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI folders_get_Count(ITaskFolderCollection *iface, LONG *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI folders_get_Item(ITaskFolderCollection *iface, VARIANT index, ITaskFolder **folder)
{
    FIXME("%p,%s,%p: stub\n", iface, debugstr_variant(&index), folder);
    return E_NOTIMPL;
}

static HRESULT WINAPI folders_get__NewEnum(ITaskFolderCollection *iface, IUnknown **penum)
{
    FIXME("%p,%p: stub\n", iface, penum);
    return E_NOTIMPL;
}

static const ITaskFolderCollectionVtbl TaskFolderCollection_vtbl =
{
    folders_QueryInterface,
    folders_AddRef,
    folders_Release,
    folders_GetTypeInfoCount,
    folders_GetTypeInfo,
    folders_GetIDsOfNames,
    folders_Invoke,
    folders_get_Count,
    folders_get_Item,
    folders_get__NewEnum
};

HRESULT TaskFolderCollection_create(const WCHAR *path, ITaskFolderCollection **obj)
{
    TaskFolderCollection *folders;

    folders = heap_alloc(sizeof(*folders));
    if (!folders) return E_OUTOFMEMORY;

    folders->ITaskFolderCollection_iface.lpVtbl = &TaskFolderCollection_vtbl;
    folders->ref = 1;
    *obj = &folders->ITaskFolderCollection_iface;

    TRACE("created %p\n", *obj);

    return S_OK;
}
