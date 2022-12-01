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
#include "winreg.h"
#include "objbase.h"
#include "taskschd.h"
#include "schrpc.h"
#include "taskschd_private.h"

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
        free(folder->path);
        free(folder);
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
    *obj = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI TaskFolder_GetTypeInfoCount(ITaskFolder *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_GetTypeInfo(ITaskFolder *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%lu,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_GetIDsOfNames(ITaskFolder *iface, REFIID riid, LPOLESTR *names,
                                                UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%lu,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_Invoke(ITaskFolder *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                         DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%ld,%s,%04lx,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_get_Name(ITaskFolder *iface, BSTR *name)
{
    TaskFolder *folder = impl_from_ITaskFolder(iface);
    const WCHAR *p_name;

    TRACE("%p,%p\n", iface, name);

    if (!name) return E_POINTER;

    p_name = wcsrchr(folder->path, '\\');
    if (!p_name)
        p_name = folder->path;
    else
        if (p_name[1] != 0) p_name++;

    *name = SysAllocString(p_name);
    if (!*name) return E_OUTOFMEMORY;

    return S_OK;
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

static HRESULT WINAPI TaskFolder_GetFolder(ITaskFolder *iface, BSTR path, ITaskFolder **new_folder)
{
    TaskFolder *folder = impl_from_ITaskFolder(iface);

    TRACE("%p,%s,%p\n", iface, debugstr_w(path), folder);

    if (!path) return E_INVALIDARG;
    if (!new_folder) return E_POINTER;

    return TaskFolder_create(folder->path, path, new_folder, FALSE);
}

static HRESULT WINAPI TaskFolder_GetFolders(ITaskFolder *iface, LONG flags, ITaskFolderCollection **folders)
{
    TaskFolder *folder = impl_from_ITaskFolder(iface);

    TRACE("%p,%lx,%p: stub\n", iface, flags, folders);

    if (!folders) return E_POINTER;

    if (flags)
        FIXME("unsupported flags %lx\n", flags);

    return TaskFolderCollection_create(folder->path, folders);
}

static inline BOOL is_variant_null(const VARIANT *var)
{
    return V_VT(var) == VT_EMPTY || V_VT(var) == VT_NULL ||
          (V_VT(var) == VT_BSTR && (V_BSTR(var) == NULL || !*V_BSTR(var)));
}

static HRESULT WINAPI TaskFolder_CreateFolder(ITaskFolder *iface, BSTR path, VARIANT sddl, ITaskFolder **new_folder)
{
    TaskFolder *folder = impl_from_ITaskFolder(iface);
    ITaskFolder *tmp_folder = NULL;
    HRESULT hr;

    TRACE("%p,%s,%s,%p\n", iface, debugstr_w(path), debugstr_variant(&sddl), folder);

    if (!path) return E_INVALIDARG;

    if (!new_folder) new_folder = &tmp_folder;

    if (!is_variant_null(&sddl))
        FIXME("security descriptor %s is ignored\n", debugstr_variant(&sddl));

    hr = TaskFolder_create(folder->path, path, new_folder, TRUE);
    if (tmp_folder)
        ITaskFolder_Release(tmp_folder);

    return hr;
}

WCHAR *get_full_path(const WCHAR *parent, const WCHAR *path)
{
    WCHAR *folder_path;
    int len = 0;

    if (path) len = lstrlenW(path);

    if (parent) len += lstrlenW(parent);

    /* +1 if parent is not '\' terminated */
    folder_path = malloc((len + 2) * sizeof(WCHAR));
    if (!folder_path) return NULL;

    folder_path[0] = 0;

    if (parent)
        lstrcpyW(folder_path, parent);

    if (path && *path)
    {
        len = lstrlenW(folder_path);
        if (!len || folder_path[len - 1] != '\\')
            lstrcatW(folder_path, L"\\");

        while (*path == '\\') path++;
        lstrcatW(folder_path, path);
    }

    len = lstrlenW(folder_path);
    if (!len)
        lstrcatW(folder_path, L"\\");

    return folder_path;
}

static HRESULT WINAPI TaskFolder_DeleteFolder(ITaskFolder *iface, BSTR name, LONG flags)
{
    TaskFolder *folder = impl_from_ITaskFolder(iface);
    WCHAR *folder_path;
    HRESULT hr;

    TRACE("%p,%s,%lx\n", iface, debugstr_w(name), flags);

    if (!name || !*name) return E_ACCESSDENIED;

    if (flags)
        FIXME("unsupported flags %lx\n", flags);

    folder_path = get_full_path(folder->path, name);
    if (!folder_path) return E_OUTOFMEMORY;

    hr = SchRpcDelete(folder_path, 0);
    free(folder_path);
    return hr;
}

static HRESULT WINAPI TaskFolder_GetTask(ITaskFolder *iface, BSTR name, IRegisteredTask **task)
{
    TaskFolder *folder = impl_from_ITaskFolder(iface);
    ITaskDefinition *taskdef;
    HRESULT hr;

    TRACE("%p,%s,%p\n", iface, debugstr_w(name), task);

    if (!task) return E_POINTER;

    hr = TaskDefinition_create(&taskdef);
    if (hr != S_OK) return hr;

    hr = RegisteredTask_create(folder->path, name, taskdef, 0, 0, task, FALSE);
    if (hr != S_OK)
        ITaskDefinition_Release(taskdef);
    return hr;
}

static HRESULT WINAPI TaskFolder_GetTasks(ITaskFolder *iface, LONG flags, IRegisteredTaskCollection **tasks)
{
    TaskFolder *folder = impl_from_ITaskFolder(iface);

    TRACE("%p,%lx,%p: stub\n", iface, flags, tasks);

    if (!tasks) return E_POINTER;

    return RegisteredTaskCollection_create(folder->path, tasks);
}

static HRESULT WINAPI TaskFolder_DeleteTask(ITaskFolder *iface, BSTR name, LONG flags)
{
    TaskFolder *folder = impl_from_ITaskFolder(iface);
    WCHAR *folder_path;
    HRESULT hr;

    TRACE("%p,%s,%lx\n", iface, debugstr_w(name), flags);

    if (!name || !*name) return E_ACCESSDENIED;

    if (flags)
        FIXME("unsupported flags %lx\n", flags);

    folder_path = get_full_path(folder->path, name);
    if (!folder_path) return E_OUTOFMEMORY;

    hr = SchRpcDelete(folder_path, 0);
    free(folder_path);
    return hr;
}

static HRESULT WINAPI TaskFolder_RegisterTask(ITaskFolder *iface, BSTR name, BSTR xml, LONG flags,
                                              VARIANT user, VARIANT password, TASK_LOGON_TYPE logon,
                                              VARIANT sddl, IRegisteredTask **task)
{
    TaskFolder *folder = impl_from_ITaskFolder(iface);
    IRegisteredTask *regtask = NULL;
    ITaskDefinition *taskdef;
    HRESULT hr;

    TRACE("%p,%s,%s,%lx,%s,%s,%d,%s,%p\n", iface, debugstr_w(name), debugstr_w(xml), flags,
          debugstr_variant(&user), debugstr_variant(&password), logon, debugstr_variant(&sddl), task);

    if (!xml) return HRESULT_FROM_WIN32(RPC_X_NULL_REF_POINTER);

    if (!task) task = &regtask;

    hr = TaskDefinition_create(&taskdef);
    if (hr != S_OK) return hr;

    hr = ITaskDefinition_put_XmlText(taskdef, xml);
    if (hr == S_OK)
        hr = RegisteredTask_create(folder->path, name, taskdef, flags, logon, task, TRUE);

    if (hr != S_OK)
        ITaskDefinition_Release(taskdef);

    if (regtask)
        IRegisteredTask_Release(regtask);

    return hr;
}

static HRESULT WINAPI TaskFolder_RegisterTaskDefinition(ITaskFolder *iface, BSTR name, ITaskDefinition *definition, LONG flags,
                                                        VARIANT user, VARIANT password, TASK_LOGON_TYPE logon,
                                                        VARIANT sddl, IRegisteredTask **task)
{
    TaskFolder *folder = impl_from_ITaskFolder(iface);
    IRegisteredTask *regtask = NULL;
    HRESULT hr;

    FIXME("%p,%s,%p,%lx,%s,%s,%d,%s,%p: stub\n", iface, debugstr_w(name), definition, flags,
          debugstr_variant(&user), debugstr_variant(&password), logon, debugstr_variant(&sddl), task);

    if (!is_variant_null(&sddl))
        FIXME("security descriptor %s is ignored\n", debugstr_variant(&sddl));

    if (!is_variant_null(&user) || !is_variant_null(&password))
        FIXME("user/password are ignored\n");

    if (!task) task = &regtask;

    ITaskDefinition_AddRef(definition);
    hr = RegisteredTask_create(folder->path, name, definition, flags, logon, task, TRUE);
    if (hr != S_OK)
        ITaskDefinition_Release(definition);

    if (regtask)
        IRegisteredTask_Release(regtask);

    return hr;
}

static HRESULT WINAPI TaskFolder_GetSecurityDescriptor(ITaskFolder *iface, LONG info, BSTR *sddl)
{
    FIXME("%p,%lx,%p: stub\n", iface, info, sddl);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskFolder_SetSecurityDescriptor(ITaskFolder *iface, BSTR sddl, LONG flags)
{
    FIXME("%p,%s,%lx: stub\n", iface, debugstr_w(sddl), flags);
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

HRESULT TaskFolder_create(const WCHAR *parent, const WCHAR *path, ITaskFolder **obj, BOOL create)
{
    TaskFolder *folder;
    WCHAR *folder_path;
    HRESULT hr;

    if (path)
    {
        int len = lstrlenW(path);
        if (len && path[len - 1] == '\\') return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
    }

    folder_path = get_full_path(parent, path);
    if (!folder_path) return E_OUTOFMEMORY;

    if (create)
    {
        hr = SchRpcCreateFolder(folder_path, NULL, 0);
    }
    else
    {
        DWORD start_index, count, i;
        TASK_NAMES names;

        start_index = 0;
        names = NULL;
        hr = SchRpcEnumFolders(folder_path, 0, &start_index, 0, &count, &names);
        if (hr == S_OK)
        {
            for (i = 0; i < count; i++)
                MIDL_user_free(names[i]);
            MIDL_user_free(names);
        }
        else
        {
            if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
        }
    }

    if (FAILED(hr))
    {
        free(folder_path);
        return hr;
    }

    folder = malloc(sizeof(*folder));
    if (!folder)
    {
        free(folder_path);
        return E_OUTOFMEMORY;
    }

    folder->ITaskFolder_iface.lpVtbl = &TaskFolder_vtbl;
    folder->ref = 1;
    folder->path = folder_path;
    *obj = &folder->ITaskFolder_iface;

    TRACE("created %p\n", *obj);

    return S_OK;
}
