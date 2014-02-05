/*
 * Copyright 2013 Dmitry Timoshkov
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
#include "initguid.h"
#include "objbase.h"
#include "taskschd.h"
#include "taskschd_private.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(taskschd);

typedef struct
{
    ITaskDefinition ITaskDefinition_iface;
    LONG ref;
} TaskDefinition;

static inline TaskDefinition *impl_from_ITaskDefinition(ITaskDefinition *iface)
{
    return CONTAINING_RECORD(iface, TaskDefinition, ITaskDefinition_iface);
}

static ULONG WINAPI TaskDefinition_AddRef(ITaskDefinition *iface)
{
    TaskDefinition *taskdef = impl_from_ITaskDefinition(iface);
    return InterlockedIncrement(&taskdef->ref);
}

static ULONG WINAPI TaskDefinition_Release(ITaskDefinition *iface)
{
    TaskDefinition *taskdef = impl_from_ITaskDefinition(iface);
    LONG ref = InterlockedDecrement(&taskdef->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        heap_free(taskdef);
    }

    return ref;
}

static HRESULT WINAPI TaskDefinition_QueryInterface(ITaskDefinition *iface, REFIID riid, void **obj)
{
    if (!riid || !obj) return E_INVALIDARG;

    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_ITaskDefinition) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        ITaskDefinition_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI TaskDefinition_GetTypeInfoCount(ITaskDefinition *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_GetTypeInfo(ITaskDefinition *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%u,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_GetIDsOfNames(ITaskDefinition *iface, REFIID riid, LPOLESTR *names,
                                                   UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%u,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_Invoke(ITaskDefinition *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                            DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%d,%s,%04x,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_get_RegistrationInfo(ITaskDefinition *iface, IRegistrationInfo **info)
{
    FIXME("%p,%p: stub\n", iface, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_put_RegistrationInfo(ITaskDefinition *iface, IRegistrationInfo *info)
{
    FIXME("%p,%p: stub\n", iface, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_get_Triggers(ITaskDefinition *iface, ITriggerCollection **triggers)
{
    FIXME("%p,%p: stub\n", iface, triggers);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_put_Triggers(ITaskDefinition *iface, ITriggerCollection *triggers)
{
    FIXME("%p,%p: stub\n", iface, triggers);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_get_Settings(ITaskDefinition *iface, ITaskSettings **settings)
{
    FIXME("%p,%p: stub\n", iface, settings);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_put_Settings(ITaskDefinition *iface, ITaskSettings *settings)
{
    FIXME("%p,%p: stub\n", iface, settings);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_get_Data(ITaskDefinition *iface, BSTR *data)
{
    FIXME("%p,%p: stub\n", iface, data);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_put_Data(ITaskDefinition *iface, BSTR data)
{
    FIXME("%p,%p: stub\n", iface, data);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_get_Principal(ITaskDefinition *iface, IPrincipal **principal)
{
    FIXME("%p,%p: stub\n", iface, principal);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_put_Principal(ITaskDefinition *iface, IPrincipal *principal)
{
    FIXME("%p,%p: stub\n", iface, principal);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_get_Actions(ITaskDefinition *iface, IActionCollection **actions)
{
    FIXME("%p,%p: stub\n", iface, actions);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_put_Actions(ITaskDefinition *iface, IActionCollection *actions)
{
    FIXME("%p,%p: stub\n", iface, actions);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_get_XmlText(ITaskDefinition *iface, BSTR *xml)
{
    FIXME("%p,%p: stub\n", iface, xml);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_put_XmlText(ITaskDefinition *iface, BSTR xml)
{
    FIXME("%p,%p: stub\n", iface, xml);
    return E_NOTIMPL;
}

static const ITaskDefinitionVtbl TaskDefinition_vtbl =
{
    TaskDefinition_QueryInterface,
    TaskDefinition_AddRef,
    TaskDefinition_Release,
    TaskDefinition_GetTypeInfoCount,
    TaskDefinition_GetTypeInfo,
    TaskDefinition_GetIDsOfNames,
    TaskDefinition_Invoke,
    TaskDefinition_get_RegistrationInfo,
    TaskDefinition_put_RegistrationInfo,
    TaskDefinition_get_Triggers,
    TaskDefinition_put_Triggers,
    TaskDefinition_get_Settings,
    TaskDefinition_put_Settings,
    TaskDefinition_get_Data,
    TaskDefinition_put_Data,
    TaskDefinition_get_Principal,
    TaskDefinition_put_Principal,
    TaskDefinition_get_Actions,
    TaskDefinition_put_Actions,
    TaskDefinition_get_XmlText,
    TaskDefinition_put_XmlText
};

static HRESULT TaskDefinition_create(ITaskDefinition **obj)
{
    TaskDefinition *taskdef;

    taskdef = heap_alloc(sizeof(*taskdef));
    if (!taskdef) return E_OUTOFMEMORY;

    taskdef->ITaskDefinition_iface.lpVtbl = &TaskDefinition_vtbl;
    taskdef->ref = 1;
    *obj = &taskdef->ITaskDefinition_iface;

    TRACE("created %p\n", *obj);

    return S_OK;
}

typedef struct
{
    ITaskService ITaskService_iface;
    LONG ref;
    BOOL connected;
    WCHAR comp_name[MAX_COMPUTERNAME_LENGTH + 1];
} TaskService;

static inline TaskService *impl_from_ITaskService(ITaskService *iface)
{
    return CONTAINING_RECORD(iface, TaskService, ITaskService_iface);
}

static ULONG WINAPI TaskService_AddRef(ITaskService *iface)
{
    TaskService *task_svc = impl_from_ITaskService(iface);
    return InterlockedIncrement(&task_svc->ref);
}

static ULONG WINAPI TaskService_Release(ITaskService *iface)
{
    TaskService *task_svc = impl_from_ITaskService(iface);
    LONG ref = InterlockedDecrement(&task_svc->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        heap_free(task_svc);
    }

    return ref;
}

static HRESULT WINAPI TaskService_QueryInterface(ITaskService *iface, REFIID riid, void **obj)
{
    if (!riid || !obj) return E_INVALIDARG;

    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_ITaskService) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        ITaskService_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI TaskService_GetTypeInfoCount(ITaskService *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskService_GetTypeInfo(ITaskService *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%u,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskService_GetIDsOfNames(ITaskService *iface, REFIID riid, LPOLESTR *names,
                                                UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%u,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskService_Invoke(ITaskService *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                         DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%d,%s,%04x,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskService_GetFolder(ITaskService *iface, BSTR path, ITaskFolder **folder)
{
    TaskService *task_svc = impl_from_ITaskService(iface);

    TRACE("%p,%s,%p\n", iface, debugstr_w(path), folder);

    if (!folder) return E_POINTER;

    if (!task_svc->connected)
        return HRESULT_FROM_WIN32(ERROR_ONLY_IF_CONNECTED);

    return TaskFolder_create(path, NULL, folder, FALSE);
}

static HRESULT WINAPI TaskService_GetRunningTasks(ITaskService *iface, LONG flags, IRunningTaskCollection **tasks)
{
    FIXME("%p,%x,%p: stub\n", iface, flags, tasks);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskService_NewTask(ITaskService *iface, DWORD flags, ITaskDefinition **definition)
{
    TaskService *task_svc = impl_from_ITaskService(iface);

    TRACE("%p,%x,%p\n", iface, flags, definition);

    if (!definition) return E_POINTER;

    if (!task_svc->connected)
        return HRESULT_FROM_WIN32(ERROR_ONLY_IF_CONNECTED);

    if (flags)
        FIXME("unsupported flags %x\n", flags);

    return TaskDefinition_create(definition);
}

static inline BOOL is_variant_null(const VARIANT *var)
{
    return V_VT(var) == VT_EMPTY || V_VT(var) == VT_NULL ||
          (V_VT(var) == VT_BSTR && (V_BSTR(var) == NULL || !*V_BSTR(var)));
}

static HRESULT WINAPI TaskService_Connect(ITaskService *iface, VARIANT server, VARIANT user, VARIANT domain, VARIANT password)
{
    TaskService *task_svc = impl_from_ITaskService(iface);
    WCHAR comp_name[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD len;

    TRACE("%p,%s,%s,%s,%s\n", iface, debugstr_variant(&server), debugstr_variant(&user),
          debugstr_variant(&domain), debugstr_variant(&password));

    if (!is_variant_null(&user) || !is_variant_null(&domain) || !is_variant_null(&password))
        FIXME("user/domain/password are ignored\n");

    len = sizeof(comp_name)/sizeof(comp_name[0]);
    if (!GetComputerNameW(comp_name, &len))
        return HRESULT_FROM_WIN32(GetLastError());

    if (!is_variant_null(&server))
    {
        const WCHAR *server_name;

        if (V_VT(&server) != VT_BSTR)
        {
            FIXME("server variant type %d is not supported\n", V_VT(&server));
            return HRESULT_FROM_WIN32(ERROR_BAD_NETPATH);
        }

        /* skip UNC prefix if any */
        server_name = V_BSTR(&server);
        if (server_name[0] == '\\' && server_name[1] == '\\')
            server_name += 2;

        if (strcmpiW(server_name, comp_name))
        {
            FIXME("connection to remote server %s is not supported\n", debugstr_w(V_BSTR(&server)));
            return HRESULT_FROM_WIN32(ERROR_BAD_NETPATH);
        }
    }

    strcpyW(task_svc->comp_name, comp_name);
    task_svc->connected = TRUE;

    return S_OK;
}

static HRESULT WINAPI TaskService_get_Connected(ITaskService *iface, VARIANT_BOOL *connected)
{
    TaskService *task_svc = impl_from_ITaskService(iface);

    TRACE("%p,%p\n", iface, connected);

    if (!connected) return E_POINTER;

    *connected = task_svc->connected ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

static HRESULT WINAPI TaskService_get_TargetServer(ITaskService *iface, BSTR *server)
{
    TaskService *task_svc = impl_from_ITaskService(iface);

    TRACE("%p,%p\n", iface, server);

    if (!server) return E_POINTER;

    if (!task_svc->connected)
        return HRESULT_FROM_WIN32(ERROR_ONLY_IF_CONNECTED);

    *server = SysAllocString(task_svc->comp_name);
    if (!*server) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI TaskService_get_ConnectedUser(ITaskService *iface, BSTR *user)
{
    FIXME("%p,%p: stub\n", iface, user);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskService_get_ConnectedDomain(ITaskService *iface, BSTR *domain)
{
    FIXME("%p,%p: stub\n", iface, domain);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskService_get_HighestVersion(ITaskService *iface, DWORD *version)
{
    FIXME("%p,%p: stub\n", iface, version);
    return E_NOTIMPL;
}

static const ITaskServiceVtbl TaskService_vtbl =
{
    TaskService_QueryInterface,
    TaskService_AddRef,
    TaskService_Release,
    TaskService_GetTypeInfoCount,
    TaskService_GetTypeInfo,
    TaskService_GetIDsOfNames,
    TaskService_Invoke,
    TaskService_GetFolder,
    TaskService_GetRunningTasks,
    TaskService_NewTask,
    TaskService_Connect,
    TaskService_get_Connected,
    TaskService_get_TargetServer,
    TaskService_get_ConnectedUser,
    TaskService_get_ConnectedDomain,
    TaskService_get_HighestVersion
};

HRESULT TaskService_create(void **obj)
{
    TaskService *task_svc;

    task_svc = heap_alloc(sizeof(*task_svc));
    if (!task_svc) return E_OUTOFMEMORY;

    task_svc->ITaskService_iface.lpVtbl = &TaskService_vtbl;
    task_svc->ref = 1;
    task_svc->connected = FALSE;
    *obj = &task_svc->ITaskService_iface;

    TRACE("created %p\n", *obj);

    return S_OK;
}
