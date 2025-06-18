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
#include "schrpc.h"
#include "taskschd_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(taskschd);

typedef struct
{
    ITaskFolderCollection ITaskFolderCollection_iface;
    LONG ref;
    WCHAR *path;
    TASK_NAMES list;
    DWORD count;
} TaskFolderCollection;

static HRESULT NewEnum_create(TaskFolderCollection *folders, IUnknown **obj);

static inline TaskFolderCollection *impl_from_ITaskFolderCollection(ITaskFolderCollection *iface)
{
    return CONTAINING_RECORD(iface, TaskFolderCollection, ITaskFolderCollection_iface);
}

static ULONG WINAPI folders_AddRef(ITaskFolderCollection *iface)
{
    TaskFolderCollection *folders = impl_from_ITaskFolderCollection(iface);
    return InterlockedIncrement(&folders->ref);
}

static void free_list(LPWSTR *list, DWORD count)
{
    LONG i;

    for (i = 0; i < count; i++)
        MIDL_user_free(list[i]);

    MIDL_user_free(list);
}

static ULONG WINAPI folders_Release(ITaskFolderCollection *iface)
{
    TaskFolderCollection *folders = impl_from_ITaskFolderCollection(iface);
    LONG ref = InterlockedDecrement(&folders->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        free_list(folders->list, folders->count);
        free(folders->path);
        free(folders);
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
    *obj = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI folders_GetTypeInfoCount(ITaskFolderCollection *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI folders_GetTypeInfo(ITaskFolderCollection *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%lu,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI folders_GetIDsOfNames(ITaskFolderCollection *iface, REFIID riid, LPOLESTR *names,
                                                UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%lu,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI folders_Invoke(ITaskFolderCollection *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                         DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%ld,%s,%04lx,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI folders_get_Count(ITaskFolderCollection *iface, LONG *count)
{
    TaskFolderCollection *folders = impl_from_ITaskFolderCollection(iface);

    TRACE("%p,%p\n", iface, count);

    if (!count) return E_POINTER;

    *count = folders->count;

    return S_OK;
}

static LONG get_var_int(const VARIANT *var)
{
    switch(V_VT(var))
    {
    case VT_I1:
    case VT_UI1:
        return V_UI1(var);

    case VT_I2:
    case VT_UI2:
        return V_UI2(var);

    case VT_I4:
    case VT_UI4:
        return V_UI4(var);

    case VT_I8:
    case VT_UI8:
        return V_UI8(var);

    case VT_INT:
    case VT_UINT:
        return V_UINT(var);

    default:
        FIXME("unsupported variant type %d\n", V_VT(var));
        return 0;
    }
}

static HRESULT WINAPI folders_get_Item(ITaskFolderCollection *iface, VARIANT index, ITaskFolder **folder)
{
    TaskFolderCollection *folders = impl_from_ITaskFolderCollection(iface);
    LONG idx;

    TRACE("%p,%s,%p\n", iface, debugstr_variant(&index), folder);

    if (!folder) return E_POINTER;

    if (V_VT(&index) == VT_BSTR)
        return TaskFolder_create(folders->path, V_BSTR(&index), folder, FALSE);

    idx = get_var_int(&index);
    /* collections are 1 based */
    if (idx < 1 || idx > folders->count)
        return E_INVALIDARG;

    return TaskFolder_create(folders->path, folders->list[idx - 1], folder, FALSE);
}

static HRESULT WINAPI folders_get__NewEnum(ITaskFolderCollection *iface, IUnknown **penum)
{
    TaskFolderCollection *folders = impl_from_ITaskFolderCollection(iface);

    TRACE("%p,%p\n", iface, penum);

    if (!penum) return E_POINTER;

    return NewEnum_create(folders, penum);
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
    HRESULT hr;
    TASK_NAMES list;
    DWORD start_index, count;

    start_index = 0;
    list = NULL;
    hr = SchRpcEnumFolders(path, 0, &start_index, 0, &count, &list);
    if (hr != S_OK) return hr;

    folders = malloc(sizeof(*folders));
    if (!folders)
    {
        free_list(list, count);
        return E_OUTOFMEMORY;
    }

    folders->ITaskFolderCollection_iface.lpVtbl = &TaskFolderCollection_vtbl;
    folders->ref = 1;
    if (!(folders->path = wcsdup(path)))
    {
        free(folders);
        free_list(list, count);
        return E_OUTOFMEMORY;
    }
    folders->count = count;
    folders->list = list;
    *obj = &folders->ITaskFolderCollection_iface;

    TRACE("created %p\n", *obj);

    return S_OK;
}

typedef struct
{
    IEnumVARIANT IEnumVARIANT_iface;
    LONG ref, pos;
    TaskFolderCollection *folders;
} EnumVARIANT;

static inline EnumVARIANT *impl_from_IEnumVARIANT(IEnumVARIANT *iface)
{
    return CONTAINING_RECORD(iface, EnumVARIANT, IEnumVARIANT_iface);
}

static HRESULT WINAPI enumvar_QueryInterface(IEnumVARIANT *iface, REFIID riid, void **obj)
{
    if (!riid || !obj) return E_INVALIDARG;

    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IEnumVARIANT) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        IEnumVARIANT_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI enumvar_AddRef(IEnumVARIANT *iface)
{
    EnumVARIANT *enumvar = impl_from_IEnumVARIANT(iface);
    return InterlockedIncrement(&enumvar->ref);
}

static ULONG WINAPI enumvar_Release(IEnumVARIANT *iface)
{
    EnumVARIANT *enumvar = impl_from_IEnumVARIANT(iface);
    LONG ref = InterlockedDecrement(&enumvar->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        ITaskFolderCollection_Release(&enumvar->folders->ITaskFolderCollection_iface);
        free(enumvar);
    }

    return ref;
}

static HRESULT WINAPI enumvar_Next(IEnumVARIANT *iface, ULONG celt, VARIANT *var, ULONG *fetched)
{
    EnumVARIANT *enumvar = impl_from_IEnumVARIANT(iface);
    LONG i;

    TRACE("%p,%lu,%p,%p\n", iface, celt, var, fetched);

    for (i = 0; i < celt && enumvar->pos < enumvar->folders->count; i++)
    {
        ITaskFolder *folder;
        HRESULT hr;

        hr = TaskFolder_create(enumvar->folders->path, enumvar->folders->list[enumvar->pos++], &folder, FALSE);
        if (hr) return hr;

        if (!var)
        {
            ITaskFolder_Release(folder);
            return E_POINTER;
        }

        V_VT(&var[i]) = VT_DISPATCH;
        V_DISPATCH(&var[i]) = (IDispatch *)folder;
    }

    if (fetched) *fetched = i;

    return i == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI enumvar_Skip(IEnumVARIANT *iface, ULONG celt)
{
    EnumVARIANT *enumvar = impl_from_IEnumVARIANT(iface);

    TRACE("%p,%lu\n", iface, celt);

    enumvar->pos += celt;

    if (enumvar->pos > enumvar->folders->count)
    {
        enumvar->pos = enumvar->folders->count;
        return S_FALSE;
    }

    return S_OK;
}

static HRESULT WINAPI enumvar_Reset(IEnumVARIANT *iface)
{
    EnumVARIANT *enumvar = impl_from_IEnumVARIANT(iface);

    TRACE("%p\n", iface);

    enumvar->pos = 0;

    return S_OK;
}

static HRESULT WINAPI enumvar_Clone(IEnumVARIANT *iface, IEnumVARIANT **penum)
{
    EnumVARIANT *enumvar = impl_from_IEnumVARIANT(iface);

    TRACE("%p,%p\n", iface, penum);

    return NewEnum_create(enumvar->folders, (IUnknown **)penum);
}

static const struct IEnumVARIANTVtbl EnumVARIANT_vtbl =
{
    enumvar_QueryInterface,
    enumvar_AddRef,
    enumvar_Release,
    enumvar_Next,
    enumvar_Skip,
    enumvar_Reset,
    enumvar_Clone
};

static HRESULT NewEnum_create(TaskFolderCollection *folders, IUnknown **obj)
{
    EnumVARIANT *enumvar;

    enumvar = malloc(sizeof(*enumvar));
    if (!enumvar) return E_OUTOFMEMORY;

    enumvar->IEnumVARIANT_iface.lpVtbl = &EnumVARIANT_vtbl;
    enumvar->ref = 1;
    enumvar->pos = 0;
    enumvar->folders = folders;
    ITaskFolderCollection_AddRef(&folders->ITaskFolderCollection_iface);

    *obj = (IUnknown *)&enumvar->IEnumVARIANT_iface;

    TRACE("created %p\n", *obj);

    return S_OK;
}
