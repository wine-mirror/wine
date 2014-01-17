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
    ITaskFolder ITaskFolder_iface;
    LONG ref;
    WCHAR *path;
} TaskFolder;

static inline TaskFolder *impl_from_ITaskFolder(ITaskFolder *iface)
{
    return CONTAINING_RECORD(iface, TaskFolder, ITaskFolder_iface);
}

static ULONG WINAPI TaskFolder_AddRef(ITaskFolder *iface)
{
    TaskFolder *folder = impl_from_ITaskFolder(iface);
    return InterlockedIncrement(&folder->ref);
}

static ULONG WINAPI TaskFolder_Release(ITaskFolder *iface)
{
    TaskFolder *folder = impl_from_ITaskFolder(iface);
    LONG ref = InterlockedDecrement(&folder->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        HeapFree(GetProcessHeap(), 0, folder->path);
        HeapFree(GetProcessHeap(), 0, folder);
    }

    return ref;
}

static HRESULT WINAPI TaskFolder_QueryInterface(ITaskFolder *iface, REFIID riid, void **obj)
{
    if (!riid || !obj) return E_INVALIDARG;

    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_ITaskFolder) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        ITaskFolder_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static HRESULT WINAPI TaskFolder_GetTypeInfoCount(ITaskFolder *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_GetTypeInfo(ITaskFolder *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%u,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_GetIDsOfNames(ITaskFolder *iface, REFIID riid, LPOLESTR *names,
                                                UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%u,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_Invoke(ITaskFolder *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                         DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%d,%s,%04x,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_get_Name(ITaskFolder *iface, BSTR *name)
{
    FIXME("%p,%p: stub\n", iface, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_get_Path(ITaskFolder *iface, BSTR *path)
{
    TaskFolder *folder = impl_from_ITaskFolder(iface);

    TRACE("%p,%p\n", iface, path);

    if (!path) return E_POINTER;

    *path = SysAllocString(folder->path);
    if (!*path) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI TaskFolder_GetFolder(ITaskFolder *iface, BSTR path, ITaskFolder **folder)
{
    FIXME("%p,%s,%p: stub\n", iface, debugstr_w(path), folder);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_GetFolders(ITaskFolder *iface, LONG flags, ITaskFolderCollection **folders)
{
    FIXME("%p,%x,%p: stub\n", iface, flags, folders);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_CreateFolder(ITaskFolder *iface, BSTR name, VARIANT sddl, ITaskFolder **folder)
{
    FIXME("%p,%s,%s,%p: stub\n", iface, debugstr_w(name), debugstr_variant(&sddl), folder);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_DeleteFolder(ITaskFolder *iface, BSTR name, LONG flags)
{
    FIXME("%p,%s,%x: stub\n", iface, debugstr_w(name), flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_GetTask(ITaskFolder *iface, BSTR path, IRegisteredTask **task)
{
    FIXME("%p,%s,%p: stub\n", iface, debugstr_w(path), task);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_GetTasks(ITaskFolder *iface, LONG flags, IRegisteredTaskCollection **tasks)
{
    FIXME("%p,%x,%p: stub\n", iface, flags, tasks);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_DeleteTask(ITaskFolder *iface, BSTR name, LONG flags)
{
    FIXME("%p,%s,%x: stub\n", iface, debugstr_w(name), flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_RegisterTask(ITaskFolder *iface, BSTR path, BSTR xml, LONG flags,
                                              VARIANT user, VARIANT password, TASK_LOGON_TYPE logon,
                                              VARIANT sddl, IRegisteredTask **task)
{
    FIXME("%p,%s,%s,%x,%s,%s,%d,%s,%p: stub\n", iface, debugstr_w(path), debugstr_w(xml), flags,
          debugstr_variant(&user), debugstr_variant(&password), logon, debugstr_variant(&sddl), task);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_RegisterTaskDefinition(ITaskFolder *iface, BSTR path, ITaskDefinition *definition, LONG flags,
                                                        VARIANT user, VARIANT password, TASK_LOGON_TYPE logon,
                                                        VARIANT sddl, IRegisteredTask **task)
{
    FIXME("%p,%s,%p,%x,%s,%s,%d,%s,%p: stub\n", iface, debugstr_w(path), definition, flags,
          debugstr_variant(&user), debugstr_variant(&password), logon, debugstr_variant(&sddl), task);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_GetSecurityDescriptor(ITaskFolder *iface, LONG info, BSTR *sddl)
{
    FIXME("%p,%x,%p: stub\n", iface, info, sddl);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_SetSecurityDescriptor(ITaskFolder *iface, BSTR sddl, LONG flags)
{
    FIXME("%p,%s,%x: stub\n", iface, debugstr_w(sddl), flags);
    return E_NOTIMPL;
}

static const ITaskFolderVtbl TaskFolder_vtbl =
{
    TaskFolder_QueryInterface,
    TaskFolder_AddRef,
    TaskFolder_Release,
    TaskFolder_GetTypeInfoCount,
    TaskFolder_GetTypeInfo,
    TaskFolder_GetIDsOfNames,
    TaskFolder_Invoke,
    TaskFolder_get_Name,
    TaskFolder_get_Path,
    TaskFolder_GetFolder,
    TaskFolder_GetFolders,
    TaskFolder_CreateFolder,
    TaskFolder_DeleteFolder,
    TaskFolder_GetTask,
    TaskFolder_GetTasks,
    TaskFolder_DeleteTask,
    TaskFolder_RegisterTask,
    TaskFolder_RegisterTaskDefinition,
    TaskFolder_GetSecurityDescriptor,
    TaskFolder_SetSecurityDescriptor
};

HRESULT TaskFolder_create(const WCHAR *parent, const WCHAR *path, ITaskFolder **obj)
{
    static const WCHAR bslash[] = { '\\', 0 };
    TaskFolder *folder;
    WCHAR *folder_path;
    int len = 0;

    if (path)
    {
        len = strlenW(path);
        if (len && path[len - 1] == '\\') return ERROR_INVALID_NAME;
    }

    if (parent) len += strlenW(parent);

    /* +1 if parent is not '\' terminated */
    folder_path = HeapAlloc(GetProcessHeap(), 0, (len + 2) * sizeof(WCHAR));
    if (!folder_path) return E_OUTOFMEMORY;

    folder_path[0] = 0;

    if (parent)
        strcpyW(folder_path, parent);

    len = strlenW(folder_path);
    if (!len || folder_path[len - 1] != '\\')
        strcatW(folder_path, bslash);

    if (path)
    {
        while (*path == '\\') path++;
        strcatW(folder_path, path);
    }

    folder = HeapAlloc(GetProcessHeap(), 0, sizeof(*folder));
    if (!folder)
    {
        HeapFree(GetProcessHeap(), 0, folder_path);
        return E_OUTOFMEMORY;
    }

    folder->ITaskFolder_iface.lpVtbl = &TaskFolder_vtbl;
    folder->ref = 1;
    folder->path = folder_path;
    *obj = &folder->ITaskFolder_iface;

    TRACE("created %p\n", *obj);

    return S_OK;
}
